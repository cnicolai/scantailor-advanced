// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ForgedImages.h"

#include <tiffio.h>

#include <algorithm>
#include <cstring>

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

}  // namespace test_support
