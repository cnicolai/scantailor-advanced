// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageCompare.h"

#include <QRgb>
#include <QSize>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include "Tolerances.h"

namespace test_support {

namespace {

constexpr int kInkThreshold = 128;  // grey value below this is ink
constexpr int kDiffGain = 8;        // colour diff image: grey = 255 - gain x largest channel difference
constexpr int kInkOnlyGrey = 64;    // bi-level diff image: ink in both images

/// One byte per pixel, 1 = ink, built from a canonical Format_Mono image.
class InkMask {
 public:
  explicit InkMask(const QImage& mono)
      : m_width(mono.width()), m_height(mono.height()), m_ink(size_t(m_width) * size_t(m_height)) {
    for (int y = 0; y < m_height; ++y) {
      const uint8_t* line = mono.constScanLine(y);
      uint8_t* out = m_ink.data() + size_t(y) * size_t(m_width);
      for (int x = 0; x < m_width; ++x) {
        out[x] = (line[x >> 3] >> (7 - (x & 7))) & 1;
      }
    }
  }

  int width() const { return m_width; }

  int height() const { return m_height; }

  uint8_t at(int x, int y) const { return m_ink[size_t(y) * size_t(m_width) + size_t(x)]; }

  const uint8_t* row(int y) const { return m_ink.data() + size_t(y) * size_t(m_width); }

 private:
  int m_width;
  int m_height;
  std::vector<uint8_t> m_ink;
};

bool hasValueNear(const InkMask& mask, int x, int y, uint8_t value) {
  for (int yy = std::max(0, y - 1); yy <= std::min(mask.height() - 1, y + 1); ++yy) {
    for (int xx = std::max(0, x - 1); xx <= std::min(mask.width() - 1, x + 1); ++xx) {
      if (mask.at(xx, yy) == value) {
        return true;
      }
    }
  }
  return false;
}

/// Pixels that differ and have no pixel of the other image's value in the 3x3 neighbourhood, in either direction.
size_t countEdgeTolerantMismatches(const InkMask& a, const InkMask& b) {
  size_t count = 0;
  for (int y = 0; y < a.height(); ++y) {
    for (int x = 0; x < a.width(); ++x) {
      const uint8_t va = a.at(x, y);
      const uint8_t vb = b.at(x, y);
      if (va != vb && (!hasValueNear(b, x, y, va) || !hasValueNear(a, x, y, vb))) {
        ++count;
      }
    }
  }
  return count;
}

/// 8-connected foreground components, iterative flood fill with an explicit stack.
size_t countComponents(const InkMask& mask) {
  const int w = mask.width();
  const int h = mask.height();
  std::vector<uint8_t> seen(size_t(w) * size_t(h), 0);
  std::vector<size_t> stack;
  size_t components = 0;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const size_t start = size_t(y) * size_t(w) + size_t(x);
      if (!mask.at(x, y) || seen[start]) {
        continue;
      }
      ++components;
      seen[start] = 1;
      stack.push_back(start);
      while (!stack.empty()) {
        const size_t current = stack.back();
        stack.pop_back();
        const int cx = int(current % size_t(w));
        const int cy = int(current / size_t(w));
        for (int ny = std::max(0, cy - 1); ny <= std::min(h - 1, cy + 1); ++ny) {
          for (int nx = std::max(0, cx - 1); nx <= std::min(w - 1, cx + 1); ++nx) {
            const size_t index = size_t(ny) * size_t(w) + size_t(nx);
            if (mask.at(nx, ny) && !seen[index]) {
              seen[index] = 1;
              stack.push_back(index);
            }
          }
        }
      }
    }
  }
  return components;
}

/// Mismatch of a(p) against b(p + (dx, dy)) over the image shrunk by the search radius on every side.
/// Stops counting once the count reaches `stopAt`, because the caller only asks whether it stays below it.
uint64_t bilevelShiftMismatch(const InkMask& a, const InkMask& b, int dx, int dy, uint64_t stopAt) {
  const int r = tolerance::kShiftSearchRadius;
  uint64_t count = 0;
  for (int y = r; y < a.height() - r; ++y) {
    const uint8_t* ra = a.row(y);
    const uint8_t* rb = b.row(y + dy);
    for (int x = r; x < a.width() - r; ++x) {
      if (ra[x] != rb[x + dx]) {
        ++count;
      }
    }
    if (count >= stopAt) {
      return count;
    }
  }
  return count;
}

/// 1 when some non-zero offset matches strictly better than (0, 0), else 0.
double bilevelShift(const InkMask& a, const InkMask& b) {
  const int r = tolerance::kShiftSearchRadius;
  if (a.width() <= 2 * r || a.height() <= 2 * r) {
    return 0.0;
  }
  const uint64_t base = bilevelShiftMismatch(a, b, 0, 0, UINT64_MAX);
  if (base == 0) {
    return 0.0;
  }
  for (int dy = -r; dy <= r; ++dy) {
    for (int dx = -r; dx <= r; ++dx) {
      if ((dx != 0 || dy != 0) && bilevelShiftMismatch(a, b, dx, dy, base) < base) {
        return 1.0;
      }
    }
  }
  return 0.0;
}

inline uint32_t absChannelSum(QRgb p, QRgb q) {
  return uint32_t(std::abs(qRed(p) - qRed(q)) + std::abs(qGreen(p) - qGreen(q)) + std::abs(qBlue(p) - qBlue(q)));
}

uint64_t colorShiftMismatch(const QImage& a, const QImage& b, int dx, int dy, uint64_t stopAt) {
  const int r = tolerance::kShiftSearchRadius;
  uint64_t sum = 0;
  for (int y = r; y < a.height() - r; ++y) {
    const QRgb* ra = reinterpret_cast<const QRgb*>(a.constScanLine(y));
    const QRgb* rb = reinterpret_cast<const QRgb*>(b.constScanLine(y + dy));
    for (int x = r; x < a.width() - r; ++x) {
      sum += absChannelSum(ra[x], rb[x + dx]);
    }
    if (sum >= stopAt) {
      return sum;
    }
  }
  return sum;
}

double colorShift(const QImage& a, const QImage& b) {
  const int r = tolerance::kShiftSearchRadius;
  if (a.width() <= 2 * r || a.height() <= 2 * r) {
    return 0.0;
  }
  const uint64_t base = colorShiftMismatch(a, b, 0, 0, UINT64_MAX);
  if (base == 0) {
    return 0.0;
  }
  for (int dy = -r; dy <= r; ++dy) {
    for (int dx = -r; dx <= r; ++dx) {
      if ((dx != 0 || dy != 0) && colorShiftMismatch(a, b, dx, dy, base) < base) {
        return 1.0;
      }
    }
  }
  return 0.0;
}

MetricResult makeMetric(const QString& name, double value, double limit) {
  return MetricResult{name, value, limit, value <= limit};
}

QImage solidImage(const QSize& size, QRgb color) {
  QImage img(std::max(1, size.width()), std::max(1, size.height()), QImage::Format_RGB32);
  img.fill(color);
  return img;
}

QImage bilevelDiff(const InkMask& a, const InkMask& b) {
  QImage diff(a.width(), a.height(), QImage::Format_RGB32);
  for (int y = 0; y < a.height(); ++y) {
    QRgb* out = reinterpret_cast<QRgb*>(diff.scanLine(y));
    for (int x = 0; x < a.width(); ++x) {
      const bool inkA = a.at(x, y) != 0;
      const bool inkB = b.at(x, y) != 0;
      if (inkA && inkB) {
        out[x] = qRgb(kInkOnlyGrey, kInkOnlyGrey, kInkOnlyGrey);
      } else if (inkB) {
        out[x] = qRgb(255, 0, 0);  // ink only in actual
      } else if (inkA) {
        out[x] = qRgb(0, 0, 255);  // ink only in expected
      } else {
        out[x] = qRgb(255, 255, 255);
      }
    }
  }
  return diff;
}

QImage colorDiff(const QImage& a, const QImage& b) {
  QImage diff(a.size(), QImage::Format_RGB32);
  for (int y = 0; y < a.height(); ++y) {
    const QRgb* ra = reinterpret_cast<const QRgb*>(a.constScanLine(y));
    const QRgb* rb = reinterpret_cast<const QRgb*>(b.constScanLine(y));
    QRgb* out = reinterpret_cast<QRgb*>(diff.scanLine(y));
    for (int x = 0; x < a.width(); ++x) {
      const int worst = std::max({std::abs(qRed(ra[x]) - qRed(rb[x])), std::abs(qGreen(ra[x]) - qGreen(rb[x])),
                                  std::abs(qBlue(ra[x]) - qBlue(rb[x]))});
      if (worst > tolerance::kColorMaxAbsError) {
        out[x] = qRgb(255, 0, 0);
      } else {
        const int grey = 255 - std::min(255, kDiffGain * worst);
        out[x] = qRgb(grey, grey, grey);
      }
    }
  }
  return diff;
}

void compareBilevel(const QImage& expected, const QImage& actual, CompareResult& result) {
  const InkMask a(expected);
  const InkMask b(actual);
  const double pixels = double(a.width()) * double(a.height());

  const size_t mismatches = countEdgeTolerantMismatches(a, b);
  result.metrics.push_back(makeMetric("edge_tolerant_xor_fraction", pixels > 0 ? double(mismatches) / pixels : 0.0,
                                      tolerance::kBilevelEdgeTolerantXorFraction));

  const double componentsA = double(countComponents(a));
  const double componentsB = double(countComponents(b));
  result.metrics.push_back(makeMetric("component_count_relative",
                                      std::abs(componentsA - componentsB) / std::max(1.0, componentsA),
                                      tolerance::kBilevelComponentCountRelative));

  result.metrics.push_back(makeMetric("shift", bilevelShift(a, b), 0.0));

  if (std::any_of(result.metrics.begin(), result.metrics.end(), [](const MetricResult& m) { return !m.pass; })) {
    result.diff = bilevelDiff(a, b);
  }
}

void compareColor(const QImage& expected, const QImage& actual, CompareResult& result) {
  uint64_t sum = 0;
  int worst = 0;
  for (int y = 0; y < expected.height(); ++y) {
    const QRgb* ra = reinterpret_cast<const QRgb*>(expected.constScanLine(y));
    const QRgb* rb = reinterpret_cast<const QRgb*>(actual.constScanLine(y));
    for (int x = 0; x < expected.width(); ++x) {
      sum += absChannelSum(ra[x], rb[x]);
      worst = std::max({worst, std::abs(qRed(ra[x]) - qRed(rb[x])), std::abs(qGreen(ra[x]) - qGreen(rb[x])),
                        std::abs(qBlue(ra[x]) - qBlue(rb[x]))});
    }
  }
  const double samples = 3.0 * double(expected.width()) * double(expected.height());
  result.metrics.push_back(makeMetric("mae", samples > 0 ? double(sum) / samples : 0.0, tolerance::kColorMeanAbsError));
  result.metrics.push_back(makeMetric("max_abs_error", double(worst), tolerance::kColorMaxAbsError));
  result.metrics.push_back(makeMetric("shift", colorShift(expected, actual), 0.0));

  if (std::any_of(result.metrics.begin(), result.metrics.end(), [](const MetricResult& m) { return !m.pass; })) {
    result.diff = colorDiff(expected, actual);
  }
}

}  // namespace

QImage canonicalImage(const QImage& image, ImageKind kind) {
  if (image.isNull()) {
    return QImage();
  }
  if (kind == ImageKind::COLOR) {
    return image.convertToFormat(QImage::Format_RGB32);
  }

  const QImage grey = image.convertToFormat(QImage::Format_Grayscale8);
  QImage mono(grey.width(), grey.height(), QImage::Format_Mono);
  mono.setColorTable({qRgb(255, 255, 255), qRgb(0, 0, 0)});  // index 1 = black = ink
  mono.fill(0);
  for (int y = 0; y < grey.height(); ++y) {
    const uint8_t* src = grey.constScanLine(y);
    uint8_t* dst = mono.scanLine(y);
    for (int x = 0; x < grey.width(); ++x) {
      if (src[x] < kInkThreshold) {
        dst[x >> 3] |= uint8_t(0x80 >> (x & 7));
      }
    }
  }
  return mono;
}

CompareResult compareImages(const QImage& expected, const QImage& actual, ImageKind kind) {
  const QImage a = canonicalImage(expected, kind);
  const QImage b = canonicalImage(actual, kind);

  CompareResult result{true, {}, QImage()};
  const bool sameSize = a.size() == b.size();
  result.metrics.push_back(makeMetric("size", sameSize ? 0.0 : 1.0, 0.0));
  if (!sameSize) {
    result.pass = false;
    result.diff = solidImage(a.size(), qRgb(255, 0, 0));
    return result;
  }

  if (kind == ImageKind::BILEVEL) {
    compareBilevel(a, b, result);
  } else {
    compareColor(a, b, result);
  }
  result.pass = std::all_of(result.metrics.begin(), result.metrics.end(), [](const MetricResult& m) { return m.pass; });
  return result;
}

}  // namespace test_support
