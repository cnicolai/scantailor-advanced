// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <CylindricalSurfaceDewarper.h>
#include <RasterDewarper.h>
#include <TruthMetrics.h>

#include <QColor>
#include <QImage>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <vector>

namespace dewarping {
namespace tests {
namespace {

// Same arc as in TestDistortionModel.cpp: a circular arc whose middle is `sagitta` px above its end points.
std::vector<QPointF> makeArc(const double x0,
                             const double x1,
                             const double yEnd,
                             const double sagitta,
                             const int count) {
  const double halfWidth = (x1 - x0) / 2.0;
  const double xMid = (x0 + x1) / 2.0;
  const double radius = (halfWidth * halfWidth + sagitta * sagitta) / (2.0 * sagitta);
  std::vector<QPointF> points;
  for (int i = 0; i < count; ++i) {
    const double x = x0 + (x1 - x0) * i / (count - 1);
    const double dx = x - xMid;
    points.emplace_back(x, yEnd - (std::sqrt(radius * radius - dx * dx) - (radius - sagitta)));
  }
  return points;
}

const double kDepthPerception = 2.0;
// Smallest residual of the drawn line before dewarping that counts as visibly curved.
const double kMinInputCurvaturePx = 3.0;
// Start bound for dewarped lines at 300 dpi is 1.5 px, then tightened from the measured worst case, recorded in
// .planning/phases/02-golden-baselines/records/unit-tests.md; the bound is twice that rounded up to the step of 0.5 px.
const double kRasterStraightnessBoundPx = 0.5;

struct Surface {
  std::vector<QPointF> top = makeArc(100, 900, 200, 30, 40);
  std::vector<QPointF> bottom = makeArc(100, 900, 1000, 10, 40);
  CylindricalSurfaceDewarper dewarper{top, bottom, kDepthPerception};
};

double distance(const QPointF& a, const QPointF& b) {
  return std::hypot(a.x() - b.x(), a.y() - b.y());
}

// Draws a one-pixel-thick dark curve, the image of the dewarped line crvY = const, into a white RGB32 image.
QImage drawImageOfHorizontalLine(const Surface& surface, const double crvY, const QSize& size) {
  QImage img(size, QImage::Format_RGB32);
  img.fill(Qt::white);
  const int steps = 4000;
  for (int i = 0; i <= steps; ++i) {
    const QPointF pt = surface.dewarper.mapToWarpedSpace(QPointF(double(i) / steps, crvY));
    const int x = static_cast<int>(std::floor(pt.x() + 0.5));
    const int y = static_cast<int>(std::floor(pt.y() + 0.5));
    if (x >= 0 && x < size.width() && y >= 0 && y < size.height()) {
      img.setPixel(x, y, qRgb(0, 0, 0));
    }
  }
  return img;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(DewarpingCylindricalDewarperTestSuite)

BOOST_AUTO_TEST_CASE(test_round_trip_within_1e9) {
  const Surface surface;
  double worst = 0.0;
  for (int i = 1; i <= 10; ++i) {
    for (int j = 1; j <= 10; ++j) {
      const QPointF crv(i / 11.0, j / 11.0);
      const QPointF back = surface.dewarper.mapToDewarpedSpace(surface.dewarper.mapToWarpedSpace(crv));
      worst = std::max(worst, distance(crv, back));
    }
  }
  BOOST_TEST_MESSAGE("DEWARP-MEASURE round_trip_max_error=" << worst);
  BOOST_CHECK_LE(worst, 1e-9);
}

BOOST_AUTO_TEST_CASE(test_directrix_endpoints_map_to_unit_corners) {
  const Surface surface;
  const CylindricalSurfaceDewarper& dw = surface.dewarper;
  // Normalised x is the arc length over the whole directrix scaled to 1; y is 0 on the first and 1 on the
  // second directrix.
  struct Corner {
    QPointF image;
    QPointF expected;
  };
  const Corner corners[] = {{surface.top.front(), QPointF(0, 0)},
                            {surface.top.back(), QPointF(1, 0)},
                            {surface.bottom.front(), QPointF(0, 1)},
                            {surface.bottom.back(), QPointF(1, 1)}};
  double worst = 0.0;
  for (const Corner& corner : corners) {
    worst = std::max(worst, distance(dw.mapToDewarpedSpace(corner.image), corner.expected));
  }
  BOOST_TEST_MESSAGE("DEWARP-MEASURE corner_max_error=" << worst);
  BOOST_CHECK_LE(worst, 1e-6);
  // A curved directrix is longer than its chord, which is one in the plane coordinates.
  BOOST_CHECK_GT(dw.directrixArcLength(), 1.0);
}

BOOST_AUTO_TEST_CASE(test_raster_dewarp_output_size) {
  const Surface surface;
  const QImage src = drawImageOfHorizontalLine(surface, 0.5, QSize(1000, 1200));
  const QSize sizes[] = {QSize(800, 800), QSize(321, 457), QSize(64, 32)};
  for (const QSize& size : sizes) {
    const QImage dst
        = RasterDewarper::dewarp(src, size, surface.dewarper, QRectF(QPointF(0, 0), QSizeF(size)), QColor(Qt::white));
    BOOST_CHECK(dst.size() == size);
    BOOST_CHECK(!dst.isNull());
  }
}

BOOST_AUTO_TEST_CASE(test_raster_dewarp_straightens_curve) {
  const Surface surface;
  const QSize dstSize(800, 800);
  const QRect region(40, 40, 720, 720);

  // Lines at a quarter, half and three quarters of the way between the directrices.
  for (const double crvY : {0.25, 0.5, 0.75}) {
    const QImage src = drawImageOfHorizontalLine(surface, crvY, QSize(1000, 1200));

    // The input line must be visibly curved, otherwise a small residual afterwards proves nothing.
    const test_support::StraightnessResult before
        = test_support::straightnessResidual(src, QRect(100, 100, 800, 900), 1, 8);
    BOOST_REQUIRE(before.valid);
    BOOST_CHECK_GT(before.maxResidualPx, kMinInputCurvaturePx);

    const QImage dst = RasterDewarper::dewarp(src, dstSize, surface.dewarper, QRectF(QPointF(0, 0), QSizeF(dstSize)),
                                              QColor(Qt::white));
    const test_support::StraightnessResult after = test_support::straightnessResidual(dst, region, 1, 8);
    BOOST_REQUIRE(after.valid);
    BOOST_TEST_MESSAGE("DEWARP-MEASURE crv_y=" << crvY << " raster_residual_before=" << before.maxResidualPx
                                               << " raster_residual_after=" << after.maxResidualPx);
    BOOST_CHECK_LE(after.maxResidualPx, kRasterStraightnessBoundPx);
  }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace dewarping
