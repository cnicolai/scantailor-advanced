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
#include <QSize>
#include <QString>
#include <boost/test/unit_test.hpp>
#include <limits>
#include <vector>

using test_support::forgedJpeg;
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
  return ImageLoader::load(*openBuffer(buffer, bytes), page);
}

// The forgery must parse (so libtiff accepts it and the claimed size is what the reader sees), and the reader
// must refuse it with a null image, quickly and without growing the peak footprint.
void checkRefusedWithoutAllocating(const ForgedTiffPage& page) {
  const QByteArray bytes = forgedTiff({page});
  BOOST_REQUIRE(!bytes.isEmpty());

  const std::vector<QSize> sizes = pageSizes(bytes);
  BOOST_REQUIRE_EQUAL(sizes.size(), size_t(1));
  BOOST_CHECK(sizes[0] == QSize(int(page.width), int(page.height)));

  const qint64 peakBefore = test_support::peakPhysFootprintBytes();
  BOOST_REQUIRE(peakBefore >= 0);
  QElapsedTimer timer;
  timer.start();
  const QImage image = readPage(bytes, 0);
  const qint64 elapsedMs = timer.elapsed();
  const qint64 peakAfter = test_support::peakPhysFootprintBytes();

  BOOST_CHECK(image.isNull());
  BOOST_CHECK_LT(peakAfter - peakBefore, kFootprintBoundBytes);
  BOOST_CHECK_LT(elapsedMs, kTimeBoundMs);
}

QImage readFirstPage(const ForgedTiffPage& page) {
  const QByteArray bytes = forgedTiff({page});
  BOOST_REQUIRE(!bytes.isEmpty());
  return readPage(bytes, 0);
}

}  // namespace

BOOST_AUTO_TEST_SUITE(CoreImageSizeLimitTestSuite)

// The refusal cases come first: the peak footprint is a high-water mark, and the boundary loads below allocate
// up to 64 MB.
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

  const qint64 peakBefore = test_support::peakPhysFootprintBytes();
  BOOST_REQUIRE(peakBefore >= 0);
  QElapsedTimer timer;
  timer.start();
  const std::vector<QSize> sizes = pageSizes(bytes);
  const qint64 elapsedMs = timer.elapsed();
  const qint64 peakAfter = test_support::peakPhysFootprintBytes();

  BOOST_REQUIRE_EQUAL(sizes.size(), size_t(1));
  BOOST_CHECK(sizes[0] == QSize(65500, 65500));
  BOOST_CHECK(ImageSizeLimit::exceeds(sizes[0]));
  BOOST_CHECK_LT(peakAfter - peakBefore, kFootprintBoundBytes);
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
  // Inside the limit the loader still decompresses as before and reports the true size.
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

BOOST_AUTO_TEST_CASE(test_tiff_side_boundary) {
  const QImage widest = readFirstPage({65535, 1, 1, 1, true});
  BOOST_CHECK(!widest.isNull());
  BOOST_CHECK(widest.size() == QSize(65535, 1));

  // Refused on the header alone, whatever the page holds.
  BOOST_CHECK(readFirstPage({65536, 1, 1, 1, false}).isNull());
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
