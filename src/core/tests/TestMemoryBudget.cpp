// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ImageId.h>
#include <ImageMetadata.h>
#include <MemoryBudget.h>
#include <PageId.h>
#include <PageInfo.h>
#include <PageSequence.h>

#include <QSize>
#include <boost/test/unit_test.hpp>

namespace {

constexpr qint64 k64GiB = 68719476736;
constexpr qint64 k16GiB = 17179869184;

ImageMetadata metadata(const int width, const int height, const int dpi) {
  return ImageMetadata(QSize(width, height), Dpi(dpi, dpi));
}

PageInfo page(const char* path, const ImageMetadata& meta) {
  return PageInfo(PageId(ImageId(path, 0), PageId::SINGLE_PAGE), meta, 1, false, false);
}

// Source size and dpi of the reference pages and the D-08 run page.
struct Reference {
  int width;
  int height;
  int dpi;
  int caps64;
  int caps16;
};

}  // namespace

BOOST_AUTO_TEST_SUITE(CoreMemoryBudgetTestSuite)

BOOST_AUTO_TEST_CASE(test_page_pixel_estimate) {
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(metadata(2480, 3508, 300)), 34799360);
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(metadata(4961, 7016, 600)), 34806376);
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(metadata(7016, 9921, 600)), 69605736);
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(metadata(14031, 19843, 1200)), 278417133);
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(metadata(8000, 9200, 2400)), 73600000);
  // The D-08 run page.
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(metadata(12000, 13800, 3600)), 165600000);
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(ImageMetadata(QSize(1000, 1000), Dpi(300, 600))), 2000000);
  // A missing dpi counts as 300.
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(ImageMetadata(QSize(1000, 1000), Dpi())), 4000000);
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(ImageMetadata(QSize(), Dpi(300, 300))), 0);
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(ImageMetadata(QSize(0, 100), Dpi(300, 300))), 0);
  // The estimate is clamped to 2^40 px, so the arithmetic cannot overflow.
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(metadata(2000000000, 2000000000, 1)),
                    MemoryBudget::kMaxEstimatePixels);
}

BOOST_AUTO_TEST_CASE(test_page_pixel_estimate_at_the_output_dpi) {
  const ImageMetadata a4 = metadata(2480, 3508, 300);
  // A null Dpi, or a side that is not positive, means the default of 600.
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(a4, Dpi()), MemoryBudget::pagePixelEstimate(a4));
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(a4, Dpi(600, 600)), 34799360);
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(a4, Dpi(0, 1200)), 69598720);
  // A higher output dpi grows the estimate with its square: 9920 x 14032 px at 1200 dpi.
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(a4, Dpi(1200, 1200)), 139197440);
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(a4, Dpi(1200, 600)), 69598720);
  // Below the source dpi the source is the larger of the two.
  BOOST_CHECK_EQUAL(MemoryBudget::pagePixelEstimate(a4, Dpi(150, 150)), 2480 * 3508);
  // The cap follows: 24 pages fit at 600 dpi on 64 GiB, 6 at 1200 dpi.
  BOOST_CHECK_EQUAL(MemoryBudget::threadsForMemory(k64GiB, MemoryBudget::pagePixelEstimate(a4)), 24);
  BOOST_CHECK_EQUAL(MemoryBudget::threadsForMemory(k64GiB, MemoryBudget::pagePixelEstimate(a4, Dpi(1200, 1200))), 6);
}

BOOST_AUTO_TEST_CASE(test_threads_for_memory_table) {
  const Reference rows[] = {
      {2480, 3508, 300, 24, 6},   {4961, 7016, 600, 24, 6},  {7016, 9921, 600, 12, 3},   {9921, 14031, 1200, 6, 1},
      {14031, 19843, 1200, 3, 1}, {8000, 9200, 2400, 11, 2}, {12000, 13800, 3600, 5, 1},
  };
  for (const Reference& row : rows) {
    const qint64 pixels = MemoryBudget::pagePixelEstimate(metadata(row.width, row.height, row.dpi));
    BOOST_CHECK_MESSAGE(MemoryBudget::threadsForMemory(k64GiB, pixels) == row.caps64,
                        row.width << " x " << row.height << " at " << row.dpi << " dpi on 64 GiB gives "
                                  << MemoryBudget::threadsForMemory(k64GiB, pixels) << ", expected " << row.caps64);
    BOOST_CHECK_MESSAGE(MemoryBudget::threadsForMemory(k16GiB, pixels) == row.caps16,
                        row.width << " x " << row.height << " at " << row.dpi << " dpi on 16 GiB gives "
                                  << MemoryBudget::threadsForMemory(k16GiB, pixels) << ", expected " << row.caps16);
  }
}

BOOST_AUTO_TEST_CASE(test_threads_for_memory_boundaries) {
  BOOST_CHECK_EQUAL(MemoryBudget::threadsForMemory(240000000, 1000000), 3);
  BOOST_CHECK_EQUAL(MemoryBudget::threadsForMemory(239999998, 1000000), 2);
  BOOST_CHECK_EQUAL(MemoryBudget::threadsForMemory(240000001, 1000000), 3);
  BOOST_CHECK_EQUAL(MemoryBudget::threadsForMemory(1000, 1000000), 1);
}

BOOST_AUTO_TEST_CASE(test_threads_for_memory_no_cap) {
  BOOST_CHECK_EQUAL(MemoryBudget::threadsForMemory(0, 1000000), MemoryBudget::kNoCap);
  BOOST_CHECK_EQUAL(MemoryBudget::threadsForMemory(-1, 1000000), MemoryBudget::kNoCap);
  BOOST_CHECK_EQUAL(MemoryBudget::threadsForMemory(k64GiB, 0), MemoryBudget::kNoCap);
  BOOST_CHECK_EQUAL(MemoryBudget::threadsForMemory(k64GiB, -1), MemoryBudget::kNoCap);
  BOOST_CHECK_EQUAL(MemoryBudget::threadsForMemory(qint64(1) << 62, 1), MemoryBudget::kNoCap);
  BOOST_CHECK_EQUAL(MemoryBudget::threadsForMemory(k64GiB, qint64(1) << 62), 1);
}

BOOST_AUTO_TEST_CASE(test_effective_threads) {
  BOOST_CHECK_EQUAL(MemoryBudget::effectiveThreads(10, 10, MemoryBudget::kNoCap), 10);
  BOOST_CHECK_EQUAL(MemoryBudget::effectiveThreads(10, 4, 2), 2);
  BOOST_CHECK_EQUAL(MemoryBudget::effectiveThreads(10, 4, 6), 4);
  BOOST_CHECK_EQUAL(MemoryBudget::effectiveThreads(3, 8, 6), 3);
  BOOST_CHECK_EQUAL(MemoryBudget::effectiveThreads(10, 0, 5), 1);
  BOOST_CHECK_EQUAL(MemoryBudget::effectiveThreads(10, 10, 0), 1);
}

BOOST_AUTO_TEST_CASE(test_largest_page_pixels) {
  PageSequence sequence;
  BOOST_CHECK_EQUAL(MemoryBudget::largestPagePixels(sequence), 0);
  sequence.append(page("/a4", metadata(2480, 3508, 300)));
  sequence.append(page("/a3", metadata(7016, 9921, 600)));
  BOOST_CHECK_EQUAL(MemoryBudget::largestPagePixels(sequence), 69605736);
}

BOOST_AUTO_TEST_CASE(test_largest_page_pixels_uses_the_output_dpi_of_each_page) {
  PageSequence sequence;
  sequence.append(page("/a4", metadata(2480, 3508, 300)));
  sequence.append(page("/small", metadata(1000, 1000, 300)));
  const auto dpiOf = [](const PageInfo& info) {
    return info.imageId().filePath() == QString("/a4") ? Dpi(1200, 1200) : Dpi(600, 600);
  };
  BOOST_CHECK_EQUAL(MemoryBudget::largestPagePixels(sequence, dpiOf), 139197440);
  // No callback, or a callback that knows nothing: the default of 600 dpi.
  BOOST_CHECK_EQUAL(MemoryBudget::largestPagePixels(sequence), 34799360);
  BOOST_CHECK_EQUAL(MemoryBudget::largestPagePixels(sequence, [](const PageInfo&) { return Dpi(); }), 34799360);
  // The page's own dpi decides which page is the largest.
  const auto smallHigh = [](const PageInfo& info) {
    return info.imageId().filePath() == QString("/small") ? Dpi(2400, 2400) : Dpi(300, 300);
  };
  BOOST_CHECK_EQUAL(MemoryBudget::largestPagePixels(sequence, smallHigh), 64000000);

  BOOST_CHECK_EQUAL(MemoryBudget::batchMemoryCap(sequence, dpiOf),
                    MemoryBudget::threadsForMemory(MemoryBudget::physicalMemoryBytes(), 139197440));
}

BOOST_AUTO_TEST_CASE(test_batch_memory_cap) {
  PageSequence sequence;
  BOOST_CHECK_EQUAL(MemoryBudget::batchMemoryCap(sequence), MemoryBudget::kNoCap);
  sequence.append(page("/a4", metadata(2480, 3508, 300)));
  sequence.append(page("/a3", metadata(7016, 9921, 600)));
  BOOST_CHECK_EQUAL(MemoryBudget::batchMemoryCap(sequence),
                    MemoryBudget::threadsForMemory(MemoryBudget::physicalMemoryBytes(), 69605736));
}

BOOST_AUTO_TEST_CASE(test_physical_memory_is_reported) {
  BOOST_CHECK_GT(MemoryBudget::physicalMemoryBytes(), qint64(1) << 30);
}

BOOST_AUTO_TEST_SUITE_END()
