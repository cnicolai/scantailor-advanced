// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "TruthMetrics.h"

#include <QRgb>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace test_support {

namespace {

const int kInkThreshold = 128;        // grey below this is ink
const int kBackgroundThreshold = 200;  // grey below this is not background
const int kChromaThreshold = 30;       // max - min of R, G, B above this is not background
const int kBandWidthDivisor = 50;      // a band row holds at least 1/50 = 2% of the width in ink

QImage asRgb32(const QImage& image) {
  return image.format() == QImage::Format_RGB32 ? image : image.convertToFormat(QImage::Format_RGB32);
}

/// Ink state of every pixel, row-major.
struct InkMap {
  int width = 0;
  int height = 0;
  std::vector<uint8_t> ink;

  bool at(const int x, const int y) const {
    return x >= 0 && y >= 0 && x < width && y < height
           && ink[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] != 0;
  }
};

InkMap inkMapOf(const QImage& image) {
  InkMap map;
  if (image.isNull()) {
    return map;
  }
  const QImage rgb = asRgb32(image);
  map.width = rgb.width();
  map.height = rgb.height();
  map.ink.resize(static_cast<size_t>(map.width) * static_cast<size_t>(map.height));
  for (int y = 0; y < map.height; ++y) {
    const QRgb* const line = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));
    uint8_t* const out = &map.ink[static_cast<size_t>(y) * static_cast<size_t>(map.width)];
    for (int x = 0; x < map.width; ++x) {
      out[x] = qGray(line[x]) < kInkThreshold ? 1 : 0;
    }
  }
  return map;
}

long long countInk(const InkMap& map) {
  long long count = 0;
  for (const uint8_t v : map.ink) {
    count += v;
  }
  return count;
}

QRect inkBox(const InkMap& map) {
  int minX = map.width;
  int minY = map.height;
  int maxX = -1;
  int maxY = -1;
  for (int y = 0; y < map.height; ++y) {
    for (int x = 0; x < map.width; ++x) {
      if (map.at(x, y)) {
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
      }
    }
  }
  return maxX < 0 ? QRect() : QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

/// Summed-area table: ink pixels in any window in O(1).
class InkIntegral {
 public:
  explicit InkIntegral(const InkMap& map) : m_width(map.width), m_height(map.height) {
    m_sums.assign(static_cast<size_t>(m_width + 1) * static_cast<size_t>(m_height + 1), 0);
    for (int y = 0; y < m_height; ++y) {
      int rowSum = 0;
      for (int x = 0; x < m_width; ++x) {
        rowSum += map.at(x, y) ? 1 : 0;
        m_sums[index(x + 1, y + 1)] = m_sums[index(x + 1, y)] + rowSum;
      }
    }
  }

  /// Ink pixels in [cx - r, cx + r] x [cy - r, cy + r], the part outside the image counting as paper.
  int inkInWindow(const int cx, const int cy, const int r) const {
    const int x0 = std::max(0, cx - r);
    const int y0 = std::max(0, cy - r);
    const int x1 = std::min(m_width, cx + r + 1);
    const int y1 = std::min(m_height, cy + r + 1);
    if (x0 >= x1 || y0 >= y1) {
      return 0;
    }
    return m_sums[index(x1, y1)] - m_sums[index(x0, y1)] - m_sums[index(x1, y0)] + m_sums[index(x0, y0)];
  }

  /// True if a pixel with the given ink state lies within Chebyshev distance r of (cx, cy).
  bool hasValueNear(const bool ink, const int cx, const int cy, const int r) const {
    const int inkCount = inkInWindow(cx, cy, r);
    const int area = (2 * r + 1) * (2 * r + 1);
    return ink ? inkCount > 0 : inkCount < area;
  }

 private:
  size_t index(const int x, const int y) const {
    return static_cast<size_t>(y) * static_cast<size_t>(m_width + 1) + static_cast<size_t>(x);
  }

  int m_width;
  int m_height;
  std::vector<int> m_sums;
};

struct Band {
  int top;
  int bottom;  // inclusive
};

/// Ink pixels per row of a strip [x0, x1] x [y0, y1].
std::vector<int> rowInk(const InkMap& map, const int x0, const int x1, const int y0, const int y1) {
  std::vector<int> counts;
  for (int y = y0; y <= y1; ++y) {
    int count = 0;
    for (int x = x0; x <= x1; ++x) {
      count += map.at(x, y) ? 1 : 0;
    }
    counts.push_back(count);
  }
  return counts;
}

/// Runs of rows holding at least 2% of the strip width in ink.
std::vector<Band> bandsOf(const std::vector<int>& counts, const int firstRow, const int stripWidth) {
  std::vector<Band> bands;
  bool inBand = false;
  for (size_t i = 0; i < counts.size(); ++i) {
    const bool dense = counts[i] >= 1 && counts[i] * kBandWidthDivisor >= stripWidth;
    const int row = firstRow + static_cast<int>(i);
    if (dense && !inBand) {
      bands.push_back(Band{row, row});
      inBand = true;
    } else if (dense) {
      bands.back().bottom = row;
    } else {
      inBand = false;
    }
  }
  return bands;
}

QRect clippedRegion(const QImage& image, const QRect& region) {
  return image.isNull() ? QRect() : region.intersected(QRect(0, 0, image.width(), image.height()));
}

int roundToInt(const double v) {
  return static_cast<int>(std::floor(v + 0.5));
}

StraightnessResult invalidStraightness(const int stripsUsed) {
  // The residual of an unusable measurement is infinite so that it cannot pass a tolerance by accident.
  return StraightnessResult{false, std::numeric_limits<double>::infinity(), 0, stripsUsed};
}

}  // namespace

double inkFraction(const QImage& image) {
  if (image.isNull()) {
    return 0.0;
  }
  const InkMap map = inkMapOf(image);
  return static_cast<double>(countInk(map)) / (static_cast<double>(map.width) * static_cast<double>(map.height));
}

QRect nonBackgroundBox(const QImage& image) {
  if (image.isNull()) {
    return QRect();
  }
  const QImage rgb = asRgb32(image);
  int minX = rgb.width();
  int minY = rgb.height();
  int maxX = -1;
  int maxY = -1;
  for (int y = 0; y < rgb.height(); ++y) {
    const QRgb* const line = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));
    for (int x = 0; x < rgb.width(); ++x) {
      const int r = qRed(line[x]);
      const int g = qGreen(line[x]);
      const int b = qBlue(line[x]);
      const int chroma = std::max(r, std::max(g, b)) - std::min(r, std::min(g, b));
      if (qGray(line[x]) < kBackgroundThreshold || chroma > kChromaThreshold) {
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
      }
    }
  }
  return maxX < 0 ? QRect() : QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

int countHorizontalBands(const QImage& image, const QRect& region) {
  const QRect r = clippedRegion(image, region);
  if (r.isEmpty()) {
    return 0;
  }
  const InkMap map = inkMapOf(image);
  return static_cast<int>(bandsOf(rowInk(map, r.left(), r.right(), r.top(), r.bottom()), r.top(), r.width()).size());
}

double boxIoU(const QRectF& a, const QRectF& b) {
  const double width = std::min(a.right(), b.right()) - std::max(a.left(), b.left());
  const double height = std::min(a.bottom(), b.bottom()) - std::max(a.top(), b.top());
  const double intersection = (width > 0.0 && height > 0.0) ? width * height : 0.0;
  const double areaA = std::max(0.0, a.width()) * std::max(0.0, a.height());
  const double areaB = std::max(0.0, b.width()) * std::max(0.0, b.height());
  const double unionArea = areaA + areaB - intersection;
  return unionArea > 0.0 ? intersection / unionArea : 0.0;
}

double centredBoxIoU(const QRectF& detected,
                     const QPointF& detectedFrameCentre,
                     const QRectF& truth,
                     const QPointF& truthFrameCentre) {
  return boxIoU(detected.translated(-detectedFrameCentre), truth.translated(-truthFrameCentre));
}

Registration registerMask(const QImage& output, const QImage& truthMask, const int searchRadius) {
  const InkMap out = inkMapOf(output);
  const InkMap truth = inkMapOf(truthMask);

  // Where the truth ink pixels are, for the overlap count at each candidate offset.
  std::vector<QPoint> truthInk;
  for (int y = 0; y < truth.height; ++y) {
    for (int x = 0; x < truth.width; ++x) {
      if (truth.at(x, y)) {
        truthInk.push_back(QPoint(x, y));
      }
    }
  }
  const long long outInk = countInk(out);
  const long long truthCount = static_cast<long long>(truthInk.size());

  QPoint start(0, 0);
  const QRect outBox = nonBackgroundBox(output);
  const QRect truthBox = inkBox(truth);
  if (!outBox.isNull() && !truthBox.isNull()) {
    const QPointF delta = QRectF(outBox).center() - QRectF(truthBox).center();
    start = QPoint(roundToInt(delta.x()), roundToInt(delta.y()));
  }

  const int radius = std::max(0, searchRadius);
  Registration best{start, std::numeric_limits<long long>::max()};
  int bestDistance = 0;
  for (int dy = -radius; dy <= radius; ++dy) {
    for (int dx = -radius; dx <= radius; ++dx) {
      const QPoint offset = start + QPoint(dx, dy);
      long long overlap = 0;
      for (const QPoint& p : truthInk) {
        overlap += out.at(p.x() + offset.x(), p.y() + offset.y()) ? 1 : 0;
      }
      const long long mismatch = outInk + truthCount - 2 * overlap;
      const int distance = dx * dx + dy * dy;
      if (mismatch < best.mismatch || (mismatch == best.mismatch && distance < bestDistance)) {
        best = Registration{offset, mismatch};
        bestDistance = distance;
      }
    }
  }
  return best;
}

double truthMismatchFraction(const QImage& output,
                             const QImage& truthMask,
                             const QPoint& offset,
                             const int edgeRadius) {
  const InkMap out = inkMapOf(output);
  const InkMap truth = inkMapOf(truthMask);
  const long long truthInk = countInk(truth);
  if (truthInk == 0) {
    return 1.0;
  }
  const InkIntegral outIntegral(out);
  const InkIntegral truthIntegral(truth);
  const int radius = std::max(0, edgeRadius);

  // Every pixel of the output and of the translated truth mask.
  const QRect domain = QRect(0, 0, out.width, out.height).united(QRect(offset, QSize(truth.width, truth.height)));
  long long mismatches = 0;
  for (int y = domain.top(); y <= domain.bottom(); ++y) {
    for (int x = domain.left(); x <= domain.right(); ++x) {
      const int tx = x - offset.x();
      const int ty = y - offset.y();
      const bool outputInk = out.at(x, y);
      const bool truthIsInk = truth.at(tx, ty);
      if (outputInk == truthIsInk) {
        continue;
      }
      // Tolerated when the output's value is found near the pixel in the truth and the truth's value is
      // found near it in the output.
      const bool truthHasOutputValue = truthIntegral.hasValueNear(outputInk, tx, ty, radius);
      const bool outputHasTruthValue = outIntegral.hasValueNear(truthIsInk, x, y, radius);
      if (!truthHasOutputValue || !outputHasTruthValue) {
        ++mismatches;
      }
    }
  }
  return static_cast<double>(mismatches) / static_cast<double>(truthInk);
}

StraightnessResult straightnessResidual(const QImage& image,
                                        const QRect& textRegion,
                                        const int expectedLines,
                                        const int strips) {
  const QRect region = clippedRegion(image, textRegion);
  if (region.isEmpty() || expectedLines < 1 || strips < 2 || region.width() < strips) {
    return invalidStraightness(0);
  }
  const InkMap map = inkMapOf(image);

  // Per line: the strip centres and the ink-weighted centre rows of the line in each usable strip.
  std::vector<std::vector<double>> xs(static_cast<size_t>(expectedLines));
  std::vector<std::vector<double>> ys(static_cast<size_t>(expectedLines));
  int usable = 0;
  for (int i = 0; i < strips; ++i) {
    const int x0 = region.left() + (i * region.width()) / strips;
    const int x1 = region.left() + ((i + 1) * region.width()) / strips - 1;
    const std::vector<int> counts = rowInk(map, x0, x1, region.top(), region.bottom());
    const std::vector<Band> bands = bandsOf(counts, region.top(), x1 - x0 + 1);
    if (static_cast<int>(bands.size()) != expectedLines) {
      continue;
    }
    ++usable;
    for (int k = 0; k < expectedLines; ++k) {
      double weighted = 0.0;
      double total = 0.0;
      for (int row = bands[static_cast<size_t>(k)].top; row <= bands[static_cast<size_t>(k)].bottom; ++row) {
        const double weight = counts[static_cast<size_t>(row - region.top())];
        weighted += weight * row;
        total += weight;
      }
      xs[static_cast<size_t>(k)].push_back((x0 + x1) / 2.0);
      ys[static_cast<size_t>(k)].push_back(weighted / total);
    }
  }
  if (usable < 2) {
    return invalidStraightness(usable);
  }

  double maxResidual = 0.0;
  for (int k = 0; k < expectedLines; ++k) {
    const std::vector<double>& x = xs[static_cast<size_t>(k)];
    const std::vector<double>& y = ys[static_cast<size_t>(k)];
    const double n = static_cast<double>(x.size());
    double meanX = 0.0;
    double meanY = 0.0;
    for (size_t j = 0; j < x.size(); ++j) {
      meanX += x[j];
      meanY += y[j];
    }
    meanX /= n;
    meanY /= n;
    double sxx = 0.0;
    double sxy = 0.0;
    for (size_t j = 0; j < x.size(); ++j) {
      sxx += (x[j] - meanX) * (x[j] - meanX);
      sxy += (x[j] - meanX) * (y[j] - meanY);
    }
    const double slope = sxx > 0.0 ? sxy / sxx : 0.0;
    for (size_t j = 0; j < x.size(); ++j) {
      const double fitted = meanY + slope * (x[j] - meanX);
      maxResidual = std::max(maxResidual, std::fabs(y[j] - fitted));
    }
  }
  return StraightnessResult{true, maxResidual, expectedLines, usable};
}

SanityCheck outputSanity(const QImage& output, const SanityExpectation& expectation) {
  if (output.isNull()) {
    return SanityCheck{false, QStringLiteral("output image is null")};
  }
  const QString size = QStringLiteral("%1x%2").arg(output.width()).arg(output.height());
  if (output.width() < expectation.minSize.width() || output.height() < expectation.minSize.height()) {
    return SanityCheck{false, QStringLiteral("size %1 is below the minimum %2x%3")
                                  .arg(size)
                                  .arg(expectation.minSize.width())
                                  .arg(expectation.minSize.height())};
  }
  const bool widthLimited = expectation.maxSize.width() > 0;
  const bool heightLimited = expectation.maxSize.height() > 0;
  if ((widthLimited && output.width() > expectation.maxSize.width())
      || (heightLimited && output.height() > expectation.maxSize.height())) {
    return SanityCheck{false, QStringLiteral("size %1 is above the maximum %2x%3")
                                  .arg(size)
                                  .arg(expectation.maxSize.width())
                                  .arg(expectation.maxSize.height())};
  }
  const double ink = inkFraction(output);
  if (ink < expectation.minInk) {
    return SanityCheck{false, QStringLiteral("ink fraction %1 is below the minimum %2 (blank output?)")
                                  .arg(ink, 0, 'f', 4)
                                  .arg(expectation.minInk, 0, 'f', 4)};
  }
  if (ink > expectation.maxInk) {
    return SanityCheck{false, QStringLiteral("ink fraction %1 is above the maximum %2 (black output?)")
                                  .arg(ink, 0, 'f', 4)
                                  .arg(expectation.maxInk, 0, 'f', 4)};
  }
  if (expectation.portrait && output.width() > output.height()) {
    return SanityCheck{false, QStringLiteral("output %1 is landscape but portrait is expected").arg(size)};
  }
  return SanityCheck{true, QString()};
}

}  // namespace test_support
