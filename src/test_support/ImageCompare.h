// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_IMAGECOMPARE_H_
#define SCANTAILOR_TEST_SUPPORT_IMAGECOMPARE_H_

#include <QImage>
#include <QString>
#include <vector>

namespace test_support {

/// COLOR covers greyscale and colour output.
enum class ImageKind { BILEVEL, COLOR };

struct MetricResult {
  QString metric;
  double value;
  double limit;
  bool pass;
};

struct CompareResult {
  bool pass;
  std::vector<MetricResult> metrics;
  /// Null when the comparison passed; otherwise an RGB32 image of the expected size.
  QImage diff;
};

/**
 * Compares decoded pixels only; encoded file bytes never matter.  Both images are converted with
 * canonicalImage() first.  The limits come from Tolerances.h.  A size mismatch fails with the single
 * metric "size".
 */
CompareResult compareImages(const QImage& expected, const QImage& actual, ImageKind kind);

/// BILEVEL -> Format_Mono with black as ink (pixel value below 128 in greyscale); COLOR -> Format_RGB32.
QImage canonicalImage(const QImage& image, ImageKind kind);

}  // namespace test_support

#endif  // SCANTAILOR_TEST_SUPPORT_IMAGECOMPARE_H_
