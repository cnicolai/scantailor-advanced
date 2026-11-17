// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_MEMORYBUDGET_H_
#define SCANTAILOR_CORE_MEMORYBUDGET_H_

#include <QtGlobal>
#include <limits>

class ImageMetadata;
class PageSequence;

/**
 * How many pages the worker pool may process at once without exhausting physical memory.
 *
 * The budget is half of the physical memory (the rest stays with macOS, the GUI, the thumbnail cache and
 * compressed memory). One page costs kBytesPerPagePixel bytes per pixel of the larger of its source and its
 * estimated output size; the cap is the budget divided by the cost of the largest page of the batch. All
 * functions are pure arithmetic apart from physicalMemoryBytes() and batchMemoryCap(), and never overflow.
 */
class MemoryBudget {
 public:
  /// Peak bytes per pixel of one page in flight: 1.2 times the worst measured 33.1 B (bytes per pixel).
  static constexpr qint64 kBytesPerPagePixel = 40;
  /// The output dpi the estimate assumes, the application's default.
  static constexpr int kAssumedOutputDpi = 600;
  /// Source dpi that counts when a page has none.
  static constexpr int kFallbackSourceDpi = 300;
  /// No cap (the result for input that says nothing about memory or pages).
  static constexpr int kNoCap = std::numeric_limits<int>::max();
  /// Upper bound of a page estimate in pixels, so that the arithmetic never overflows.
  static constexpr qint64 kMaxEstimatePixels = qint64(1) << 40;

  /// Physical memory in bytes (hw.memsize), 0 if the system does not tell.
  static qint64 physicalMemoryBytes();

  /// Pixels of the larger of the source and the output at kAssumedOutputDpi; 0 for an empty size.
  static qint64 pagePixelEstimate(const ImageMetadata& metadata);

  /// The largest pagePixelEstimate of the pages; 0 for none.
  static qint64 largestPagePixels(const PageSequence& pages);

  /// Pages in flight that fit the budget, in [1, kNoCap]; kNoCap for non-positive input.
  static int threadsForMemory(qint64 physicalBytes, qint64 pagePixels);

  /// The one function that turns a page sequence into the batch cap.
  static int batchMemoryCap(const PageSequence& pages);

  /// The threads the pool runs: the smallest of the three, at least 1.
  static int effectiveThreads(int idealThreads, int sliderThreads, int memoryCap);
};


#endif  // ifndef SCANTAILOR_CORE_MEMORYBUDGET_H_
