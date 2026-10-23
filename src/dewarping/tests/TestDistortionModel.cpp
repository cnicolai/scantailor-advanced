// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Curve.h>
#include <DistortionModel.h>
#include <DistortionModelBuilder.h>
#include <Rng.h>
#include <VecNT.h>

#include <QDomDocument>
#include <QDomElement>
#include <QLineF>
#include <QPointF>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <vector>

namespace dewarping {
namespace tests {
namespace {

// A circular arc through (x0, yEnd) and (x1, yEnd) whose middle is `sagitta` px above the end points (smaller y),
// sampled at `count` points with equal steps in x.  Built from integer steps and std::sqrt only.
std::vector<QPointF> makeArc(const double x0,
                             const double x1,
                             const double yEnd,
                             const double sagitta,
                             const int count,
                             const double noisePx = 0.0,
                             test_support::Rng* rng = nullptr) {
  const double halfWidth = (x1 - x0) / 2.0;
  const double xMid = (x0 + x1) / 2.0;
  const double radius = (halfWidth * halfWidth + sagitta * sagitta) / (2.0 * sagitta);
  std::vector<QPointF> points;
  for (int i = 0; i < count; ++i) {
    const double x = x0 + (x1 - x0) * i / (count - 1);
    const double dx = x - xMid;
    double y = yEnd - (std::sqrt(radius * radius - dx * dx) - (radius - sagitta));
    if (rng) {
      y += noisePx * (static_cast<double>(rng->below(2001)) - 1000.0) / 1000.0;
    }
    points.emplace_back(x, y);
  }
  return points;
}

DistortionModel makeModelFromArcs() {
  DistortionModel model;
  model.setTopCurve(Curve(makeArc(100, 900, 200, 30, 40)));
  model.setBottomCurve(Curve(makeArc(100, 900, 1000, 30, 40)));
  return model;
}

DistortionModel buildFromCurves(const uint64_t seed) {
  test_support::Rng rng(seed);
  DistortionModelBuilder builder(Vec2d(0, 1));
  builder.setVerticalBounds(QLineF(100, 0, 100, 1200), QLineF(900, 0, 900, 1200));
  for (int i = 0; i < 5; ++i) {
    builder.addHorizontalCurve(makeArc(100, 900, 250 + 175 * i, 30, 41, 0.5, &rng));
  }
  return builder.tryBuildModel();
}

}  // namespace

BOOST_AUTO_TEST_SUITE(DewarpingDistortionModelTestSuite)

BOOST_AUTO_TEST_CASE(test_model_from_arcs_is_valid) {
  const DistortionModel model = makeModelFromArcs();
  BOOST_CHECK(model.topCurve().isValid());
  BOOST_CHECK(model.bottomCurve().isValid());
  BOOST_CHECK(model.isValid());
  BOOST_CHECK_EQUAL(model.topCurve().polyline().size(), 40u);
}

BOOST_AUTO_TEST_CASE(test_degenerate_model_is_invalid) {
  BOOST_CHECK(!DistortionModel().isValid());

  DistortionModel singlePoints;
  singlePoints.setTopCurve(Curve(std::vector<QPointF>{QPointF(100, 200)}));
  singlePoints.setBottomCurve(Curve(std::vector<QPointF>{QPointF(100, 1000)}));
  BOOST_CHECK(!singlePoints.topCurve().isValid());
  BOOST_CHECK(!singlePoints.isValid());

  // Valid curves whose end points do not form a convex quadrilateral: the bottom curve runs the other way round.
  DistortionModel crossed;
  crossed.setTopCurve(Curve(makeArc(100, 900, 200, 30, 40)));
  crossed.setBottomCurve(Curve(makeArc(900, 100, 1000, 30, 40)));
  BOOST_CHECK(!crossed.isValid());

  // An invalid model has no XML form.
  QDomDocument doc;
  BOOST_CHECK(singlePoints.toXml(doc, "distortion-model").isNull());
}

BOOST_AUTO_TEST_CASE(test_xml_round_trip_matches) {
  const DistortionModel model = makeModelFromArcs();

  QDomDocument doc;
  const QDomElement el = model.toXml(doc, "distortion-model");
  BOOST_REQUIRE(!el.isNull());
  doc.appendChild(el);

  const DistortionModel restored(el);
  BOOST_CHECK(restored.isValid());
  BOOST_CHECK(restored.matches(model));
  BOOST_CHECK(model.matches(restored));

  // The polyline is stored with float precision, so matches() cannot be a bit comparison; a model that is 5 px
  // off must still differ.
  DistortionModel shifted;
  std::vector<QPointF> top = makeArc(100, 900, 200, 30, 40);
  for (QPointF& pt : top) {
    pt.setY(pt.y() + 5.0);
  }
  shifted.setTopCurve(Curve(top));
  shifted.setBottomCurve(model.bottomCurve());
  BOOST_REQUIRE(shifted.isValid());
  BOOST_CHECK(!shifted.matches(model));
}

BOOST_AUTO_TEST_CASE(test_builder_from_curves_is_valid_and_deterministic) {
  const DistortionModel first = buildFromCurves(11);
  const DistortionModel second = buildFromCurves(11);
  BOOST_CHECK(first.isValid());
  BOOST_CHECK(second.isValid());
  BOOST_CHECK(first.matches(second));

  // Top and bottom come from the outermost curves, in the right order.
  if (first.isValid()) {
    BOOST_CHECK_LT(first.topCurve().polyline().front().y(), first.bottomCurve().polyline().front().y());
  }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace dewarping
