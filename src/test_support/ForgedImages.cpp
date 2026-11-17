// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ForgedImages.h"

#include <tiffio.h>
#include <zlib.h>

#include <QBuffer>
#include <QImage>
#include <algorithm>
#include <cstdlib>
#include <cstring>

extern "C" {
#include <jpeglib.h>
}

namespace test_support {

namespace {

// libtiff writes into this buffer through TIFFClientOpen; a seek past the end grows it with zero bytes.
struct MemoryFile {
  QByteArray data;
  qint64 pos = 0;
};

tmsize_t memRead(thandle_t handle, void* buf, tmsize_t size) {
  MemoryFile* file = static_cast<MemoryFile*>(handle);
  const qint64 available = std::max<qint64>(0, file->data.size() - file->pos);
  const qint64 count = std::min<qint64>(size, available);
  if (count > 0) {
    std::memcpy(buf, file->data.constData() + file->pos, size_t(count));
  }
  file->pos += count;
  return tmsize_t(count);
}

tmsize_t memWrite(thandle_t handle, void* buf, tmsize_t size) {
  MemoryFile* file = static_cast<MemoryFile*>(handle);
  if (file->pos + size > file->data.size()) {
    file->data.resize(qsizetype(file->pos + size));
  }
  std::memcpy(file->data.data() + file->pos, buf, size_t(size));
  file->pos += size;
  return size;
}

toff_t memSeek(thandle_t handle, toff_t offset, int whence) {
  MemoryFile* file = static_cast<MemoryFile*>(handle);
  qint64 target = 0;
  switch (whence) {
    case SEEK_SET:
      target = qint64(offset);
      break;
    case SEEK_CUR:
      target = file->pos + qint64(offset);
      break;
    case SEEK_END:
      target = file->data.size() + qint64(offset);
      break;
    default:
      return toff_t(-1);
  }
  if (target < 0) {
    return toff_t(-1);
  }
  if (target > file->data.size()) {
    file->data.resize(qsizetype(target));
  }
  file->pos = target;
  return toff_t(target);
}

int memClose(thandle_t) {
  return 0;
}

toff_t memSize(thandle_t handle) {
  return toff_t(static_cast<MemoryFile*>(handle)->data.size());
}

int memMap(thandle_t, void**, toff_t*) {
  return 0;
}

void memUnmap(thandle_t, void*, toff_t) {}

void writePage(TIFF* tif, const ForgedTiffPage& page) {
  const int photometric = page.samplesPerPixel == 3
                              ? PHOTOMETRIC_RGB
                              : (page.bitsPerSample == 1 ? PHOTOMETRIC_MINISWHITE : PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, page.width);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, page.height);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, page.bitsPerSample);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, page.samplesPerPixel);
  TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photometric);
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);

  if (page.fullData) {
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, quint32(64));
    const tmsize_t lineBytes = tmsize_t((quint64(page.width) * page.samplesPerPixel * page.bitsPerSample + 7) / 8);
    const std::vector<unsigned char> line(size_t(lineBytes), 0);
    for (quint32 y = 0; y < page.height; ++y) {
      TIFFWriteScanline(tif, const_cast<unsigned char*>(line.data()), y);
    }
  } else {
    // One strip holds the whole claimed page, and only a few raw bytes are written into it. This avoids a
    // scanline buffer of the claimed width, which would be hundreds of megabytes for a very wide page.
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, page.height);
    unsigned char raw[16] = {};
    TIFFWriteRawStrip(tif, 0, raw, sizeof raw);
  }
  TIFFWriteDirectory(tif);
}

void putBigEndian32(QByteArray& data, const int offset, const quint32 value) {
  for (int i = 0; i < 4; ++i) {
    data[offset + i] = char((value >> (24 - 8 * i)) & 0xff);
  }
}

void putBigEndian16(QByteArray& data, const int offset, const quint16 value) {
  data[offset] = char(value >> 8);
  data[offset + 1] = char(value & 0xff);
}

}  // namespace

QByteArray forgedTiff(const std::vector<ForgedTiffPage>& pages) {
  MemoryFile file;
  TIFF* tif
      = TIFFClientOpen("forged", "wl", &file, &memRead, &memWrite, &memSeek, &memClose, &memSize, &memMap, &memUnmap);
  if (!tif) {
    return QByteArray();
  }
  for (const ForgedTiffPage& page : pages) {
    writePage(tif, page);
  }
  TIFFClose(tif);
  return file.data;
}

QByteArray forgedPng(const quint32 width, const quint32 height) {
  QImage image(1, 1, QImage::Format_Grayscale8);
  image.fill(0);
  QByteArray data;
  QBuffer buffer(&data);
  if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
    return QByteArray();
  }
  buffer.close();

  // Signature (8 bytes), IHDR length (4), "IHDR" (4), then width, height at offsets 16 and 20; the CRC at 29
  // covers the type and the 13 data bytes (offsets 12 to 28).
  if (data.size() < 33 || data.mid(12, 4) != "IHDR") {
    return QByteArray();
  }
  putBigEndian32(data, 16, width);
  putBigEndian32(data, 20, height);
  const uLong crc = crc32(crc32(0L, Z_NULL, 0), reinterpret_cast<const Bytef*>(data.constData()) + 12, 17);
  putBigEndian32(data, 29, quint32(crc));
  return data;
}

QByteArray forgedJpeg(const quint16 width, const quint16 height, const bool progressive) {
  jpeg_compress_struct compress;
  jpeg_error_mgr errorManager;
  compress.err = jpeg_std_error(&errorManager);
  jpeg_create_compress(&compress);
  unsigned char* memory = nullptr;
  unsigned long memorySize = 0;
  jpeg_mem_dest(&compress, &memory, &memorySize);
  compress.image_width = 64;
  compress.image_height = 64;
  compress.input_components = 3;
  compress.in_color_space = JCS_RGB;
  jpeg_set_defaults(&compress);
  if (progressive) {
    jpeg_simple_progression(&compress);
  }
  jpeg_start_compress(&compress, TRUE);
  unsigned char row[64 * 3] = {};
  JSAMPROW rowPointer = row;
  for (int y = 0; y < 64; ++y) {
    jpeg_write_scanlines(&compress, &rowPointer, 1);
  }
  jpeg_finish_compress(&compress);
  QByteArray data(reinterpret_cast<const char*>(memory), qsizetype(memorySize));
  jpeg_destroy_compress(&compress);
  std::free(memory);

  // Walk the marker segments after SOI; the first SOF0 (baseline) or SOF2 (progressive) holds the precision (1
  // byte), the height (2) and the width (2) behind its marker (2) and length (2).
  int pos = 2;
  while (pos + 4 <= data.size() && static_cast<unsigned char>(data[pos]) == 0xff) {
    const unsigned char marker = static_cast<unsigned char>(data[pos + 1]);
    const int length = (static_cast<unsigned char>(data[pos + 2]) << 8) | static_cast<unsigned char>(data[pos + 3]);
    if (marker == 0xc0 || marker == 0xc2) {
      if (pos + 9 > data.size()) {
        break;
      }
      putBigEndian16(data, pos + 5, height);
      putBigEndian16(data, pos + 7, width);
      return data;
    }
    pos += 2 + length;
  }
  return QByteArray();
}

}  // namespace test_support
