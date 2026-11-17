// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "MemoryBudget.h"

#include <sys/sysctl.h>
#include <sys/types.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "ImageMetadata.h"
#include "PageInfo.h"
#include "PageSequence.h"

qint64 MemoryBudget::physicalMemoryBytes() {
  uint64_t value = 0;
  size_t size = sizeof(value);
  if (sysctlbyname("hw.memsize", &value, &size, nullptr, 0) != 0) {
    return 0;
  }
  return value > static_cast<uint64_t>(std::numeric_limits<qint64>::max()) ? std::numeric_limits<qint64>::max()
                                                                           : static_cast<qint64>(value);
}

qint64 MemoryBudget::pagePixelEstimate(const ImageMetadata& metadata) {
  const QSize size = metadata.size();
  if (size.isEmpty()) {
    return 0;
  }

  const int dpiX = metadata.dpi().horizontal() > 0 ? metadata.dpi().horizontal() : kFallbackSourceDpi;
  const int dpiY = metadata.dpi().vertical() > 0 ? metadata.dpi().vertical() : kFallbackSourceDpi;

  // Doubles hold every product below 2^53 exactly, and anything above 2^40 is clamped, so nothing overflows.
  const double sourcePixels = static_cast<double>(size.width()) * size.height();
  const double outputWidth = std::ceil(static_cast<double>(size.width()) * kAssumedOutputDpi / dpiX);
  const double outputHeight = std::ceil(static_cast<double>(size.height()) * kAssumedOutputDpi / dpiY);
  // The larger of source and output: a page below the output dpi is held at both sizes (RESEARCH Pitfall 7).
  const double pixels = std::max(sourcePixels, outputWidth * outputHeight);
  if (pixels >= static_cast<double>(kMaxEstimatePixels)) {
    return kMaxEstimatePixels;
  }
  return static_cast<qint64>(pixels);
}

qint64 MemoryBudget::largestPagePixels(const PageSequence& pages) {
  qint64 largest = 0;
  for (const PageInfo& page : pages) {
    largest = std::max(largest, pagePixelEstimate(page.metadata()));
  }
  return largest;
}

int MemoryBudget::threadsForMemory(const qint64 physicalBytes, const qint64 pagePixels) {
  if (physicalBytes <= 0 || pagePixels <= 0) {
    return kNoCap;
  }

  const qint64 budgetBytes = physicalBytes / 2;
  const qint64 pageBytes = kBytesPerPagePixel * std::min(pagePixels, kMaxEstimatePixels);  // at most 40 * 2^40
  const qint64 threads = budgetBytes / pageBytes;
  return static_cast<int>(std::max<qint64>(1, std::min<qint64>(threads, kNoCap)));
}

int MemoryBudget::batchMemoryCap(const PageSequence& pages) {
  return threadsForMemory(physicalMemoryBytes(), largestPagePixels(pages));
}

int MemoryBudget::effectiveThreads(const int idealThreads, const int sliderThreads, const int memoryCap) {
  return std::max(1, std::min({idealThreads, sliderThreads, memoryCap}));
}
