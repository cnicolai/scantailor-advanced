// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <DistortionModel.h>
#include <DistortionModelBuilder.h>
#include <Dpi.h>
#include <ExpectedFailure.h>
#include <GrayImage.h>
#include <PageGenerator.h>
#include <TaskStatus.h>
#include <TextLineTracer.h>
#include <VecNT.h>

#include <QImage>
#include <QPointF>
#include <QRect>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <limits>
#include <vector>

namespace dewarping {
namespace tests {
namespace {

// The tracer never meets a cancelled task here, so a status that never cancels is all it needs (NullTaskStatus
// lives in core, which this executable does not link).
class NeverCancelled : public TaskStatus {
 public:
  void cancel() override {}
  bool isCancelled() const override { return false; }
  void throwIfCancelled() const override {}
};

// Geometry of the generated pages in input pixels at 300 dpi: a glyph is 7 dots of 5 px high, a text line pitch is
// 11 dots (PageGenerator.h).
constexpr double kGlyphHeightPx = 35.0;
constexpr double kLinePitchPx = 55.0;

// The start bound is one line pitch.  The bottom curve measured 5.18 px in Release
// (.planning/phases/02-golden-baselines/records/unit-tests.md), at most half the start bound, so its bound is twice
// that rounded up to the step of 0.5 px.
constexpr double kStartBoundPx = kLinePitchPx;
constexpr double kBottomBoundPx = 10.5;

test_support::PageSpec curvedSpec() {
  test_support::PageSpec spec;
  spec.kind = test_support::PageKind::CURVED;
  spec.dpi = 300;
  spec.seed = 1;
  spec.skewDeg = 0.0;
  return spec;
}

// Bounding box of the warped text block: the flat block outline, sampled along its edges, mapped to the input.
QRect warpedBoundingBox(const test_support::PageSpec& spec, const QRect& flatBlock) {
  // QRectF::united() ignores rectangles without area, so track the extremes by hand.
  double minX = std::numeric_limits<double>::max();
  double minY = minX;
  double maxX = std::numeric_limits<double>::lowest();
  double maxY = maxX;
  const int samples = 40;
  for (int i = 0; i <= samples; ++i) {
    const double fx = flatBlock.left() + double(flatBlock.width()) * i / samples;
    const double fy = flatBlock.top() + double(flatBlock.height()) * i / samples;
    const QPointF pts[] = {QPointF(fx, flatBlock.top()), QPointF(fx, flatBlock.bottom()), QPointF(flatBlock.left(), fy),
                           QPointF(flatBlock.right(), fy)};
    for (const QPointF& flat : pts) {
      const QPointF img = test_support::mapFlatToImage(spec, flat);
      minX = std::min(minX, img.x());
      minY = std::min(minY, img.y());
      maxX = std::max(maxX, img.x());
      maxY = std::max(maxY, img.y());
    }
  }
  return QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).toAlignedRect();
}

struct TracedPage {
  test_support::GeneratedPage page;
  DistortionModel model;
};

// The page is rendered and traced once; both cases read the result.
const TracedPage& tracedCurvedPage() {
  static const TracedPage traced = [] {
    TracedPage result;
    const test_support::PageSpec spec = curvedSpec();
    result.page = test_support::generatePage(spec);
    const QRect contentRect = warpedBoundingBox(spec, result.page.truth.textBlock);
    const imageproc::GrayImage input(result.page.image);

    DistortionModelBuilder builder(Vec2d(0, 1));
    NeverCancelled status;
    TextLineTracer::trace(input, Dpi(300, 300), contentRect, builder, status);
    result.model = builder.tryBuildModel();
    return result;
  }();
  return traced;
}

// y of the polyline at x, by linear interpolation; infinity outside the x range of the polyline (left to right).
double polylineYAt(const std::vector<QPointF>& polyline, const double x) {
  for (size_t i = 1; i < polyline.size(); ++i) {
    const QPointF& a = polyline[i - 1];
    const QPointF& b = polyline[i];
    if (x >= std::min(a.x(), b.x()) && x <= std::max(a.x(), b.x()) && a.x() != b.x()) {
      return a.y() + (b.y() - a.y()) * (x - a.x()) / (b.x() - a.x());
    }
  }
  return std::numeric_limits<double>::infinity();
}

// Largest vertical distance between a model curve and the image of a text line centre at nine evenly spaced x
// positions across the text block.
double maxCurveDistance(const std::vector<QPointF>& curve, const int lineIndex) {
  const test_support::PageSpec spec = curvedSpec();
  const test_support::GroundTruth& truth = tracedCurvedPage().page.truth;
  const double flatY = truth.lineTops.at(lineIndex) + kGlyphHeightPx / 2.0;
  double worst = 0.0;
  for (int k = 0; k < 9; ++k) {
    const double flatX = truth.textBlock.left() + double(truth.textBlock.width()) * k / 8.0;
    const QPointF img = test_support::mapFlatToImage(spec, QPointF(flatX, flatY));
    worst = std::max(worst, std::fabs(polylineYAt(curve, img.x()) - img.y()));
  }
  return worst;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(DewarpingTextLineTracerTestSuite)

BOOST_AUTO_TEST_CASE(test_tracer_builds_valid_model_from_curved_page) {
  const TracedPage& traced = tracedCurvedPage();
  BOOST_REQUIRE(!traced.page.image.isNull());
  BOOST_CHECK(traced.model.topCurve().isValid());
  BOOST_CHECK(traced.model.bottomCurve().isValid());
  BOOST_CHECK(traced.model.isValid());
}

BOOST_AUTO_TEST_CASE(test_model_curves_follow_first_and_last_lines) {
  const TracedPage& traced = tracedCurvedPage();
  BOOST_REQUIRE(traced.model.isValid());
  const test_support::GroundTruth& truth = traced.page.truth;
  BOOST_REQUIRE_EQUAL(static_cast<int>(truth.lineTops.size()), truth.lineCount);

  const double topDistance = maxCurveDistance(traced.model.topCurve().polyline(), 0);
  const double bottomDistance = maxCurveDistance(traced.model.bottomCurve().polyline(), truth.lineCount - 1);
  BOOST_TEST_MESSAGE("DEWARP-MEASURE tracer_top_curve_max_distance="
                     << topDistance << " tracer_bottom_curve_max_distance=" << bottomDistance);

  // F-04-2: on this page the model's top curve follows the second text line (5.7 px from its centre) and not the
  // first (64.4 px, more than one pitch), so the first line lies outside the model.
  XFAIL_CHECK(topDistance <= kStartBoundPx,
              "model top curve skips the first text line of the curved page (seed 1): no requirement");
  BOOST_CHECK_LE(bottomDistance, kBottomBoundPx);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace dewarping
