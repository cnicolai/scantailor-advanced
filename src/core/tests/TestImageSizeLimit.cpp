// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ForgedImages.h>
#include <ImageLoader.h>
#include <ImageMetadata.h>
#include <ImageMetadataLoader.h>
#include <ImageSizeLimit.h>
#include <ProcessMemory.h>
#include <TiffReader.h>

#include <QBuffer>
#include <QElapsedTimer>
#include <QImage>
#include <QImageWriter>
#include <QSize>
#include <QString>
#include <boost/test/unit_test.hpp>
#include <limits>
#include <vector>

using test_support::forgedJpeg;
using test_support::forgedNonInterleavedJpeg;
using test_support::forgedTiff;
using test_support::ForgedTiffPage;

namespace {

constexpr qint64 kFootprintBoundBytes = qint64(64) * 1024 * 1024;
constexpr qint64 kTimeBoundMs = 1000;

// The readers close the device they are given (libtiff's close procedure), so every call gets a fresh buffer
// over the bytes, as the application opens a fresh file for every read.
QBuffer* openBuffer(QBuffer& buffer, const QByteArray& bytes) {
  buffer.setData(bytes);
  BOOST_REQUIRE(buffer.open(QIODevice::ReadOnly));
  return &buffer;
}

// Metadata of every page of the file, read from its header only.
std::vector<QSize> pageSizes(const QByteArray& bytes) {
  std::vector<QSize> sizes;
  QBuffer buffer;
  const ImageMetadataLoader::Status status = ImageMetadataLoader::load(
      *openBuffer(buffer, bytes), [&sizes](const ImageMetadata& metadata) { sizes.push_back(metadata.size()); });
  BOOST_REQUIRE(status == ImageMetadataLoader::LOADED);
  return sizes;
}

QImage readPage(const QByteArray& bytes, const int page) {
  QBuffer buffer;
  return TiffReader::readImage(*openBuffer(buffer, bytes), page);
}

QImage loadPage(const QByteArray& bytes, const int page) {
  QBuffer buffer;
  return ImageLoader::loadSource(*openBuffer(buffer, bytes), page);
}

// The loader for files the application wrote itself: no size limit.
QImage loadOwnFile(const QByteArray& bytes, const int page) {
  QBuffer buffer;
  return ImageLoader::load(*openBuffer(buffer, bytes), page);
}

// The forgery must parse (so libtiff accepts it and the claimed size is what the reader sees), and the reader
// must refuse it with a null image, quickly and without growing the footprint.
void checkRefusedWithoutAllocating(const ForgedTiffPage& page) {
  const QByteArray bytes = forgedTiff({page});
  BOOST_REQUIRE(!bytes.isEmpty());

  const std::vector<QSize> sizes = pageSizes(bytes);
  BOOST_REQUIRE_EQUAL(sizes.size(), size_t(1));
  BOOST_CHECK(sizes[0] == QSize(int(page.width), int(page.height)));

  // The current footprint is sampled during the call, not the process peak, which an earlier test case may have
  // left above anything a regression would allocate here.
  test_support::FootprintSampler sampler;
  BOOST_REQUIRE(sampler.valid());
  QElapsedTimer timer;
  timer.start();
  const QImage image = readPage(bytes, 0);
  const qint64 elapsedMs = timer.elapsed();
  const qint64 growth = sampler.stop();

  BOOST_CHECK(image.isNull());
  BOOST_CHECK_LT(growth, kFootprintBoundBytes);
  BOOST_CHECK_LT(elapsedMs, kTimeBoundMs);
}

QImage readFirstPage(const ForgedTiffPage& page) {
  const QByteArray bytes = forgedTiff({page});
  BOOST_REQUIRE(!bytes.isEmpty());
  return readPage(bytes, 0);
}

}  // namespace

BOOST_AUTO_TEST_SUITE(CoreImageSizeLimitTestSuite)

BOOST_AUTO_TEST_CASE(test_forged_mono_tiff_is_refused_without_allocating) {
  checkRefusedWithoutAllocating({100000, 100000, 1, 1, false});
}

BOOST_AUTO_TEST_CASE(test_forged_rgb_tiff_is_refused_without_allocating) {
  checkRefusedWithoutAllocating({40000, 40000, 8, 3, false});
}

BOOST_AUTO_TEST_CASE(test_forged_wide_tiff_is_refused_without_allocating) {
  // A width of 2 * 10^9 px is below 2^31 but far above the limit per side.
  checkRefusedWithoutAllocating({2000000000, 2, 1, 1, false});
}

BOOST_AUTO_TEST_CASE(test_rgb_tiff_at_the_ceiling_is_refused_without_allocating) {
  // Exactly 2^29 px: the RGB32 form would take 2^31 bytes, one more than INT_MAX.
  checkRefusedWithoutAllocating({32768, 16384, 8, 3, false});
}

BOOST_AUTO_TEST_CASE(test_largest_accepted_size_fits_int_bytes) {
  // 2^29 - 1 = 233 x 1103 x 2089 has no factor pair within 65535, so the largest accepted rectangle has
  // 2^29 - 2 px. The page is only parsed: decoding it would allocate 2 GiB.
  const QByteArray bytes = forgedTiff({{55245, 9718, 8, 3, false}});
  BOOST_REQUIRE(!bytes.isEmpty());

  const std::vector<QSize> sizes = pageSizes(bytes);
  BOOST_REQUIRE_EQUAL(sizes.size(), size_t(1));
  BOOST_CHECK(sizes[0] == QSize(55245, 9718));
  BOOST_CHECK(!ImageSizeLimit::exceeds(sizes[0]));

  const qint64 rgb32Bytes = qint64(4) * sizes[0].width() * sizes[0].height();
  BOOST_CHECK_EQUAL(rgb32Bytes, qint64(2147483640));
  BOOST_CHECK_LE(rgb32Bytes, qint64(std::numeric_limits<int>::max()));
  BOOST_CHECK_EQUAL(4 * ImageSizeLimit::kMaxPixels, qint64(2147483644));
}

BOOST_AUTO_TEST_CASE(test_forged_progressive_jpeg_metadata_is_fast_and_small) {
  // A progressive forgery is the harmful one: jpeg_start_decompress allocates the whole coefficient image of the
  // claimed size (12.3 GB for 65500 x 65500). A baseline forgery would prove nothing.
  const QByteArray bytes = forgedJpeg(65500, 65500, true);
  BOOST_REQUIRE(!bytes.isEmpty());

  test_support::FootprintSampler sampler;
  BOOST_REQUIRE(sampler.valid());
  QElapsedTimer timer;
  timer.start();
  const std::vector<QSize> sizes = pageSizes(bytes);
  const qint64 elapsedMs = timer.elapsed();
  const qint64 growth = sampler.stop();

  BOOST_REQUIRE_EQUAL(sizes.size(), size_t(1));
  BOOST_CHECK(sizes[0] == QSize(65500, 65500));
  BOOST_CHECK(ImageSizeLimit::exceeds(sizes[0]));
  BOOST_CHECK_LT(growth, kFootprintBoundBytes);
  BOOST_CHECK_LT(elapsedMs, kTimeBoundMs);
}

BOOST_AUTO_TEST_CASE(test_progressive_jpeg_inside_the_limit_is_not_decompressed_for_metadata) {
  // 65500 x 8191 px is 536,500,000 px, just inside the limit (65500 is libjpeg's largest side). The header
  // alone gives the size and the density, so reading the metadata must not run jpeg_start_decompress, which
  // would allocate the whole coefficient image (1.5 GB, measured) of a file that is a few KB on disk.
  const QByteArray bytes = forgedJpeg(65500, 8191, true);
  BOOST_REQUIRE(!bytes.isEmpty());

  test_support::FootprintSampler sampler;
  BOOST_REQUIRE(sampler.valid());
  QElapsedTimer timer;
  timer.start();
  const std::vector<QSize> sizes = pageSizes(bytes);
  const qint64 elapsedMs = timer.elapsed();
  const qint64 growth = sampler.stop();

  BOOST_REQUIRE_EQUAL(sizes.size(), size_t(1));
  BOOST_CHECK(sizes[0] == QSize(65500, 8191));
  BOOST_CHECK(!ImageSizeLimit::exceeds(sizes[0]));
  BOOST_CHECK_LT(growth, kFootprintBoundBytes);
  BOOST_CHECK_LT(elapsedMs, kTimeBoundMs);
}

BOOST_AUTO_TEST_CASE(test_non_interleaved_baseline_jpeg_inside_the_limit_is_not_decompressed_for_metadata) {
  // A baseline file whose three scans carry one component each is not progressive, but libjpeg still allocates
  // the whole coefficient image for it in jpeg_start_decompress (1.5 GB for this size, measured).
  const QByteArray bytes = forgedNonInterleavedJpeg(65500, 8191);
  BOOST_REQUIRE(!bytes.isEmpty());

  test_support::FootprintSampler sampler;
  BOOST_REQUIRE(sampler.valid());
  QElapsedTimer timer;
  timer.start();
  const std::vector<QSize> sizes = pageSizes(bytes);
  const qint64 elapsedMs = timer.elapsed();
  const qint64 growth = sampler.stop();

  BOOST_REQUIRE_EQUAL(sizes.size(), size_t(1));
  BOOST_CHECK(sizes[0] == QSize(65500, 8191));
  BOOST_CHECK(!ImageSizeLimit::exceeds(sizes[0]));
  BOOST_CHECK_LT(growth, kFootprintBoundBytes);
  BOOST_CHECK_LT(elapsedMs, kTimeBoundMs);
}

BOOST_AUTO_TEST_CASE(test_forged_baseline_jpeg_metadata_reports_claimed_size) {
  const QByteArray bytes = forgedJpeg(65500, 65500, false);
  BOOST_REQUIRE(!bytes.isEmpty());

  const std::vector<QSize> sizes = pageSizes(bytes);
  BOOST_REQUIRE_EQUAL(sizes.size(), size_t(1));
  BOOST_CHECK(sizes[0] == QSize(65500, 65500));
}

BOOST_AUTO_TEST_CASE(test_small_progressive_jpeg_metadata_is_unchanged) {
  // Inside the limit the loader reports the true size.
  const QByteArray bytes = forgedJpeg(64, 64, true);
  BOOST_REQUIRE(!bytes.isEmpty());

  const std::vector<QSize> sizes = pageSizes(bytes);
  BOOST_REQUIRE_EQUAL(sizes.size(), size_t(1));
  BOOST_CHECK(sizes[0] == QSize(64, 64));
}

BOOST_AUTO_TEST_CASE(test_exceeds_table) {
  struct Row {
    int width;
    int height;
    bool exceeds;
  };
  const Row rows[] = {
      {65535, 1, false},     {65536, 1, true},
      {1, 65536, true},      {32768, 16384, true},  // exactly 2^29 px
      {32767, 16384, false},                        // 2^29 - 16384 px
      {55245, 9718, false},                         // 2^29 - 2 px, the largest accepted rectangle
      {32768, 16385, true},  {23170, 23170, false},
      {23171, 23171, true},  {14031, 19843, false},         // A3 at 1200 dpi
      {19843, 28063, true},                                 // A2 at 1200 dpi
      {-1, 10, true},        {10, int(3000000000u), true},  // a header value of 2^31 or more arrives negative
      {0, 10, false},
  };
  for (const Row& row : rows) {
    BOOST_TEST_CONTEXT(row.width << " x " << row.height) {
      BOOST_CHECK_EQUAL(ImageSizeLimit::exceeds(QSize(row.width, row.height)), row.exceeds);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_texts) {
  BOOST_CHECK(ImageSizeLimit::sizeText(QSize(100000, 100000)) == QString::fromUtf8("100000 × 100000 px"));
  BOOST_CHECK(ImageSizeLimit::sizeText(QSize(int(3000000000u), 1)) == QString::fromUtf8("3000000000 × 1 px"));
  BOOST_CHECK(ImageSizeLimit::limitText() == QString("65535 px per side and 537 megapixels in total"));
  BOOST_CHECK(ImageSizeLimit::tooLargeText(QSize(100000, 100000))
              == QString::fromUtf8("too large: 100000 × 100000 px"));
}

BOOST_AUTO_TEST_CASE(test_oversized_pages_text) {
  const ImageMetadata small(QSize(64, 64), Dpi(300, 300));
  const ImageMetadata huge(QSize(100000, 100000), Dpi(300, 300));
  const ImageMetadata wide(QSize(70000, 1), Dpi(300, 300));

  BOOST_CHECK(ImageSizeLimit::oversizedPagesText({}).isEmpty());
  BOOST_CHECK(ImageSizeLimit::oversizedPagesText({small}).isEmpty());
  BOOST_CHECK(ImageSizeLimit::oversizedPagesText({huge}) == QString::fromUtf8("too large: 100000 × 100000 px"));
  // The first oversized page is named, counted from 1.
  BOOST_CHECK(ImageSizeLimit::oversizedPagesText({small, huge, wide})
              == QString::fromUtf8("page 2 too large: 100000 × 100000 px"));
  BOOST_CHECK(ImageSizeLimit::oversizedPagesText({small, small}).isEmpty());
}

BOOST_AUTO_TEST_CASE(test_tiff_side_boundary) {
  const QImage widest = readFirstPage({65535, 1, 1, 1, true});
  BOOST_CHECK(!widest.isNull());
  BOOST_CHECK(widest.size() == QSize(65535, 1));

  // Refused on the header alone, whatever the page holds.
  BOOST_CHECK(readFirstPage({65536, 1, 1, 1, false}).isNull());
}

BOOST_AUTO_TEST_CASE(test_own_files_are_not_limited_but_source_files_are) {
  // The application's own output may be larger than its source (an output at a high dpi), so the loader for
  // such files must not apply the limit; the loader for source scans must. The files are real and tiny: a
  // 65536 px wide line is one over the limit per side and costs 8 KB as a bilevel TIFF. A forged header is
  // never read without the limit, because that read would allocate the claimed size.
  const QByteArray tiff = forgedTiff({{65536, 1, 1, 1, true}});
  BOOST_REQUIRE(!tiff.isEmpty());
  BOOST_REQUIRE(ImageSizeLimit::exceeds(QSize(65536, 1)));

  BOOST_CHECK(loadPage(tiff, 0).isNull());
  const QImage own = loadOwnFile(tiff, 0);
  BOOST_REQUIRE(!own.isNull());
  BOOST_CHECK(own.size() == QSize(65536, 1));

  // The same through the general loader for a format Qt reads.
  QImage line(QSize(65536, 1), QImage::Format_Grayscale8);
  line.fill(Qt::white);
  QByteArray png;
  {
    QBuffer writeBuffer(&png);
    BOOST_REQUIRE(writeBuffer.open(QIODevice::WriteOnly));
    QImageWriter writer(&writeBuffer, "PNG");
    BOOST_REQUIRE(writer.write(line));
  }
  BOOST_CHECK(loadPage(png, 0).isNull());
  const QImage ownPng = loadOwnFile(png, 0);
  BOOST_REQUIRE(!ownPng.isNull());
  BOOST_CHECK(ownPng.size() == QSize(65536, 1));
}

BOOST_AUTO_TEST_CASE(test_tiff_total_boundary) {
  // 2^29 - 16384 px, 64 MB as a bilevel image.
  const QImage largest = readFirstPage({32767, 16384, 1, 1, true});
  BOOST_CHECK(!largest.isNull());
  BOOST_CHECK(largest.size() == QSize(32767, 16384));

  // Exactly 2^29 px.
  BOOST_CHECK(readFirstPage({32768, 16384, 1, 1, false}).isNull());
}

BOOST_AUTO_TEST_CASE(test_multi_page_tiff_refuses_only_the_oversized_page) {
  const QByteArray bytes = forgedTiff({{64, 64, 8, 1, true}, {100000, 100000, 1, 1, false}});
  BOOST_REQUIRE(!bytes.isEmpty());

  const std::vector<QSize> sizes = pageSizes(bytes);
  BOOST_REQUIRE_EQUAL(sizes.size(), size_t(2));
  BOOST_CHECK(sizes[1] == QSize(100000, 100000));

  const QImage first = readPage(bytes, 0);
  BOOST_CHECK(!first.isNull());
  BOOST_CHECK(first.size() == QSize(64, 64));

  BOOST_CHECK(readPage(bytes, 1).isNull());
  BOOST_CHECK(loadPage(bytes, 1).isNull());
  // The same file through the general loader: page 0 is not affected by the refusal of page 1.
  BOOST_CHECK(!loadPage(bytes, 0).isNull());
}

BOOST_AUTO_TEST_SUITE_END()
