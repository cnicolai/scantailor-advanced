// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "TruthMetrics.h"

namespace test_support {

// Interface stubs for the RED commit of plan 02-03; replaced by the implementation.
double inkFraction(const QImage&) {
  return -1.0;
}

QRect nonBackgroundBox(const QImage&) {
  return QRect();
}

int countHorizontalBands(const QImage&, const QRect&) {
  return -1;
}

double boxIoU(const QRectF&, const QRectF&) {
  return -1.0;
}

double centredBoxIoU(const QRectF&, const QPointF&, const QRectF&, const QPointF&) {
  return -1.0;
}

Registration registerMask(const QImage&, const QImage&, int) {
  return Registration{QPoint(), -1};
}

double truthMismatchFraction(const QImage&, const QImage&, const QPoint&, int) {
  return -1.0;
}

StraightnessResult straightnessResidual(const QImage&, const QRect&, int, int) {
  return StraightnessResult{true, -1.0, -1, -1};
}

SanityCheck outputSanity(const QImage&, const SanityExpectation&) {
  return SanityCheck{true, QString()};
}

}  // namespace test_support
