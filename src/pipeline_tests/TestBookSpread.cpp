// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ExpectedFailure.h>
#include <PageGenerator.h>
#include <TruthMetrics.h>

#include <QImage>
#include <QLineF>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QString>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <string>
#include <vector>

#include "HeadlessPipeline.h"

namespace {

// Ground-truth bounds.  Starting values of D-15, then set by the D-15 tightening rule from the measurements in
// records/scenario-book.md ("Tolerance decisions").
const double kAngleToleranceDeg = 0.01;  // start 0.15; worst case 0 on all eight sub-pages, so one step
// The cutter may leave the gutter by this much at any sampled height (start: gutter edges +- 2 px).  Worst case 0 (the
// cutter stays at least 80 px inside the gutter), so one step.
const double kMaxOutsideGutterPx = 0.5;
// D-15 point check: distance of the cutter from the gutter centre at mid-height.  Kept at the starting value; the
// unmodified code misses it (D-18, F-06-1).
const double kMaxMidHeightOffsetPx = 2.0;
// Kept at the starting value: worst 1 - IoU 0.0319 is above half of 0.05.
const double kMinContentIou = 0.95;

const double kAppliedSkewDeg = 1.0;
const int kOutputDpi = 600;
const int kSampleStepPx = 10;
// Sanity (D-05): the glyph ink starts within this distance of the stored hard margins, at 600 dpi.
const double kMarginSlackMm = 5.0;
const double kMillimetresPerInch = 25.4;
const double kSanityWidthSlack = 1.2;
const double kMinInkFractionOfOutput = 0.005;
const double kMaxInkFractionOfOutput = 0.5;

struct SpreadScenario {
  const char* name;
  int dpi;
  output::ColorMode colorMode;
};

const SpreadScenario kSpread300Bw = {"spread_300_bw", 300, output::BLACK_AND_WHITE};
const SpreadScenario kSpread300Color = {"spread_300_color", 300, output::COLOR_GRAYSCALE};
const SpreadScenario kSpread600Bw = {"spread_600_bw", 600, output::BLACK_AND_WHITE};
const SpreadScenario kSpread600Color = {"spread_600_color", 600, output::COLOR_GRAYSCALE};

std::string num(const double value) {
  return QString::number(value, 'g', 10).toStdString();
}

void reportMetric(const std::string& scenario, const char* metric, const double value, const std::string& bound) {
  BOOST_TEST_MESSAGE("TRUTH-METRIC suite=book scenario=" << scenario << " metric=" << metric << " value=" << num(value)
                                                         << " bound=" << bound);
}

test_support::PageSpec spreadSpec(const int dpi) {
  test_support::PageSpec spec;
  spec.kind = test_support::PageKind::SPREAD;
  spec.dpi = dpi;
  spec.seed = 1;
  spec.skewDeg = kAppliedSkewDeg;
  return spec;
}

// Generated pages are shared between the scenario cases: rendering a 600 dpi page in Debug takes seconds.
const test_support::GeneratedPage& spreadPage(const int dpi) {
  static std::vector<std::pair<int, test_support::GeneratedPage>> cache;
  for (const auto& entry : cache) {
    if (entry.first == dpi) {
      return entry.second;
    }
  }
  cache.emplace_back(dpi, test_support::generatePage(spreadSpec(dpi)));
  return cache.back().second;
}

// x of a line that is not horizontal at height y (extended beyond its end points).
double xAt(const QLineF& line, const double y) {
  return line.x1() + (y - line.y1()) * (line.x2() - line.x1()) / (line.y2() - line.y1());
}

struct SanityResult {
  bool ok;
  QString reason;
};

// Output sanity of one sub-page (D-05).  truthBlock is the ink box of the sub-page's text block in input pixels.
SanityResult subPageSanity(const SpreadScenario& scenario,
                           const test_support::GeneratedPage& page,
                           const QRect& truthBlock,
                           const SubPageResult& sub) {
  const QImage& out = sub.output;
  const double scale = static_cast<double>(kOutputDpi) / scenario.dpi;
  test_support::SanityExpectation expectation;
  expectation.minSize
      = QSize(static_cast<int>(scale * truthBlock.width()), static_cast<int>(scale * truthBlock.height()));
  // A sub-page is about half the spread wide.
  expectation.maxSize = QSize(static_cast<int>(scale * page.truth.pageSize.width() * 0.5 * kSanityWidthSlack), 0);
  expectation.minInk = kMinInkFractionOfOutput;
  expectation.maxInk = kMaxInkFractionOfOutput;
  // The hard margins can make a sub-page wider than high, so the orientation is checked on the glyph ink box below.
  expectation.portrait = false;
  const test_support::SanityCheck basic = test_support::outputSanity(out, expectation);
  if (!basic.ok) {
    return SanityResult{false, basic.reason};
  }

  const QRect box = test_support::nonBackgroundBox(out);
  if (box.isNull()) {
    return SanityResult{false, QStringLiteral("output has no non-background pixels")};
  }
  if (box.width() > box.height()) {
    return SanityResult{
        false,
        QStringLiteral("glyph ink box %1x%2 is landscape but portrait is expected").arg(box.width()).arg(box.height())};
  }
  const int bands = test_support::countHorizontalBands(out, box);
  if (std::abs(bands - page.truth.lineCount) > 1) {
    return SanityResult{false,
                        QStringLiteral("found %1 text lines, truth has %2").arg(bands).arg(page.truth.lineCount)};
  }
  const double pxPerMm = kOutputDpi / kMillimetresPerInch;
  const double slackPx = kMarginSlackMm * pxPerMm;
  const double leftExpected = sub.hardMarginLeftMM * pxPerMm;
  const double topExpected = sub.hardMarginTopMM * pxPerMm;
  if (std::fabs(box.left() - leftExpected) > slackPx || std::fabs(box.top() - topExpected) > slackPx) {
    return SanityResult{false, QStringLiteral("ink box starts at (%1, %2) px, stored margins put it near (%3, %4) px")
                                   .arg(box.left())
                                   .arg(box.top())
                                   .arg(leftExpected)
                                   .arg(topExpected)};
  }
  return SanityResult{true, QString()};
}

struct SubMeasured {
  double deskewErrorDeg = 0.0;
  double contentIou = 0.0;
  SanityResult sanity{false, QString()};
};

// Everything measured for one spread scenario.
struct SpreadMeasured {
  test_support::GeneratedPage page;
  PipelineResult result;
  QLineF cutter;
  double worstOutsideGutterPx = 0.0;  // largest distance by which the cutter is outside the gutter at a sample
  double worstGutterMarginPx = 0.0;   // smallest distance of the cutter to the nearer gutter edge, negative = outside
  double midHeightOffsetPx = 0.0;     // |cutter x - gutter centre x| at mid-height
  SubMeasured subs[2];
};

SpreadMeasured measureSpread(const SpreadScenario& scenario) {
  SpreadMeasured m;
  m.page = spreadPage(scenario.dpi);
  BOOST_REQUIRE_MESSAGE(!m.page.image.isNull(), scenario.name << ": the generator returned a null page");
  PipelineOptions options;
  options.colorMode = scenario.colorMode;
  m.result = runPipeline(m.page.image, options);
  BOOST_REQUIRE_EQUAL(m.result.layoutType, 2);  // TWO_PAGES
  BOOST_REQUIRE_EQUAL(m.result.cutters.size(), 1u);
  BOOST_REQUIRE_EQUAL(m.result.pages.size(), 2u);
  m.cutter = m.result.cutters.front();
  const QLineF& gutter = m.page.truth.gutterCentre;
  BOOST_REQUIRE_MESSAGE(m.cutter.y1() != m.cutter.y2() && gutter.y1() != gutter.y2(),
                        scenario.name << ": cutter or gutter line is horizontal");

  const double height = m.page.image.height();
  const double halfWidth = m.page.truth.gutterHalfWidthPx;
  m.worstGutterMarginPx = halfWidth;
  std::vector<double> samples;
  for (double y = 0.0; y < height; y += kSampleStepPx) {
    samples.push_back(y);
  }
  samples.push_back(height);
  for (const double y : samples) {
    const double offset = std::fabs(xAt(m.cutter, y) - xAt(gutter, y));
    m.worstGutterMarginPx = std::min(m.worstGutterMarginPx, halfWidth - offset);
    m.worstOutsideGutterPx = std::max(m.worstOutsideGutterPx, offset - halfWidth);
  }
  m.midHeightOffsetPx = std::fabs(xAt(m.cutter, height / 2.0) - xAt(gutter, height / 2.0));

  const QRect truthBlocks[2] = {m.page.truth.leftBlock, m.page.truth.rightBlock};
  // Frame centre of each truth block: select_content reports the content box relative to the centre of the
  // sub-page's bounding box in the deskewed frame, which has the flat orientation.  The same centre in the flat frame
  // is the centre of the bounding box of the sub-page polygon (the input corners and the found cutter) mapped from the
  // input to the flat frame.
  const double width = m.page.image.width();
  const QPointF polygons[2][4]
      = {{QPointF(0, 0), QPointF(xAt(m.cutter, 0.0), 0), QPointF(xAt(m.cutter, height), height), QPointF(0, height)},
         {QPointF(xAt(m.cutter, 0.0), 0), QPointF(width, 0), QPointF(width, height),
          QPointF(xAt(m.cutter, height), height)}};
  QPointF truthCentres[2];
  for (int i = 0; i < 2; ++i) {
    double minX = 1e300, maxX = -1e300, minY = 1e300, maxY = -1e300;  // QRectF::united ignores empty boxes
    for (const QPointF& corner : polygons[i]) {
      const QPointF flat = test_support::mapImageToFlat(spreadSpec(scenario.dpi), corner);
      minX = std::min(minX, flat.x());
      maxX = std::max(maxX, flat.x());
      minY = std::min(minY, flat.y());
      maxY = std::max(maxY, flat.y());
    }
    truthCentres[i] = QPointF((minX + maxX) / 2.0, (minY + maxY) / 2.0);
  }
  for (int i = 0; i < 2; ++i) {
    const SubPageResult& sub = m.result.pages[i];
    BOOST_REQUIRE_MESSAGE(!sub.output.isNull(), scenario.name << ": sub-page " << i << " has no output image");
    BOOST_REQUIRE_MESSAGE(sub.pageRect.isValid() && sub.contentRect.isValid(),
                          scenario.name << ": select_content found no boxes for sub-page " << i);
    m.subs[i].deskewErrorDeg = std::fabs(sub.deskewAngleDeg + kAppliedSkewDeg);
    m.subs[i].contentIou = test_support::centredBoxIoU(QRectF(sub.contentRect), sub.pageRect.center(),
                                                       QRectF(truthBlocks[i]), truthCentres[i]);
    m.subs[i].sanity = subPageSanity(scenario, m.page, truthBlocks[i], sub);
  }
  return m;
}

const char* sideName(const int index) {
  return index == 0 ? "left" : "right";
}

void reportSpread(const SpreadScenario& scenario, const SpreadMeasured& m) {
  const std::string name = scenario.name;
  reportMetric(name, "layout_type", m.result.layoutType, "2");
  reportMetric(name, "cutter_count", static_cast<double>(m.result.cutters.size()), "1");
  reportMetric(name, "cutter_x1_px", m.cutter.x1(), "none");
  reportMetric(name, "cutter_y1_px", m.cutter.y1(), "none");
  reportMetric(name, "cutter_x2_px", m.cutter.x2(), "none");
  reportMetric(name, "cutter_y2_px", m.cutter.y2(), "none");
  reportMetric(name, "gutter_half_width_px", m.page.truth.gutterHalfWidthPx, "none");
  reportMetric(name, "worst_outside_gutter_px", m.worstOutsideGutterPx, num(kMaxOutsideGutterPx));
  reportMetric(name, "worst_gutter_margin_px", m.worstGutterMarginPx, "none");
  reportMetric(name, "mid_height_offset_px", m.midHeightOffsetPx, num(kMaxMidHeightOffsetPx));
  for (int i = 0; i < 2; ++i) {
    const SubPageResult& sub = m.result.pages[i];
    const std::string prefix = std::string(sideName(i)) + "_";
    reportMetric(name, (prefix + "stored_deskew_deg").c_str(), sub.deskewAngleDeg, "none");
    reportMetric(name, (prefix + "deskew_error_deg").c_str(), m.subs[i].deskewErrorDeg, num(kAngleToleranceDeg));
    reportMetric(name, (prefix + "content_iou").c_str(), m.subs[i].contentIou, num(kMinContentIou));
    reportMetric(name, (prefix + "output_w_px").c_str(), sub.outputSize.width(), "none");
    reportMetric(name, (prefix + "output_h_px").c_str(), sub.outputSize.height(), "none");
    reportMetric(name, (prefix + "content_w_px").c_str(), sub.contentRect.width(), "none");
    reportMetric(name, (prefix + "content_h_px").c_str(), sub.contentRect.height(), "none");
    const QRect truthBlock = i == 0 ? m.page.truth.leftBlock : m.page.truth.rightBlock;
    reportMetric(name, (prefix + "truth_block_w_px").c_str(), truthBlock.width(), "none");
    reportMetric(name, (prefix + "truth_block_h_px").c_str(), truthBlock.height(), "none");
    reportMetric(name, (prefix + "page_rect_w_px").c_str(), sub.pageRect.width(), "none");
    reportMetric(name, (prefix + "page_rect_h_px").c_str(), sub.pageRect.height(), "none");
    BOOST_TEST_MESSAGE("TRUTH-METRIC suite=book scenario="
                       << name << " metric=" << prefix << "sanity value=" << (m.subs[i].sanity.ok ? 1 : 0)
                       << " bound=1 reason=" << m.subs[i].sanity.reason.toStdString());
  }
}

// midOffsetDefect: when not null, the D-15 mid-height check is an expected failure (D-18) with this tag.
void checkSpread(const SpreadScenario& scenario, const SpreadMeasured& m, const char* midOffsetDefect) {
  const std::string name = scenario.name;
  BOOST_CHECK_EQUAL(m.result.layoutType, 2);
  BOOST_CHECK_EQUAL(m.result.cutters.size(), 1u);
  BOOST_CHECK_MESSAGE(m.worstOutsideGutterPx <= kMaxOutsideGutterPx, name << ": the cutter leaves the gutter by "
                                                                          << num(m.worstOutsideGutterPx)
                                                                          << " px, limit " << num(kMaxOutsideGutterPx));
  const bool midHeightOk = m.midHeightOffsetPx <= kMaxMidHeightOffsetPx;
  if (midOffsetDefect != nullptr) {
    XFAIL_CHECK(midHeightOk, midOffsetDefect);
  } else {
    BOOST_CHECK_MESSAGE(midHeightOk, name << ": the cutter is " << num(m.midHeightOffsetPx)
                                          << " px from the gutter centre at mid-height, limit "
                                          << num(kMaxMidHeightOffsetPx));
  }
  for (int i = 0; i < 2; ++i) {
    const std::string side = sideName(i);
    BOOST_CHECK_MESSAGE(m.subs[i].deskewErrorDeg <= kAngleToleranceDeg,
                        name << ": " << side << " deskew error " << num(m.subs[i].deskewErrorDeg) << " deg exceeds "
                             << num(kAngleToleranceDeg));
    BOOST_CHECK_MESSAGE(m.subs[i].contentIou >= kMinContentIou, name << ": " << side << " content box IoU "
                                                                     << num(m.subs[i].contentIou) << " is below "
                                                                     << num(kMinContentIou));
    BOOST_CHECK_EQUAL(m.result.pages[i].outputDpiX, kOutputDpi);
    BOOST_CHECK_EQUAL(m.result.pages[i].outputDpiY, kOutputDpi);
  }
}

void runSpreadCase(const SpreadScenario& scenario, const char* midOffsetDefect = nullptr) {
  const SpreadMeasured m = measureSpread(scenario);
  reportSpread(scenario, m);
  checkSpread(scenario, m, midOffsetDefect);
}

}  // namespace

BOOST_AUTO_TEST_SUITE(BookSpreadTestSuite)

// F-06-1: on every spread the unmodified page_split puts its cutter inside the gutter but far from the gutter centre,
// 30.4 px at 300 dpi and 55.7 px at 600 dpi (27.9 px in 300 dpi pixels).
BOOST_AUTO_TEST_CASE(test_spread_300_bw_split) {
  runSpreadCase(kSpread300Bw,
                "page_split cutter 30.4 px from the gutter centre at mid-height on the 300 dpi spread, bound 2 px "
                "(F-06-1): no requirement");
}

BOOST_AUTO_TEST_CASE(test_spread_300_color_split) {
  runSpreadCase(kSpread300Color,
                "page_split cutter 30.4 px from the gutter centre at mid-height on the 300 dpi spread, bound 2 px "
                "(F-06-1): no requirement");
}

BOOST_AUTO_TEST_CASE(test_spread_600_bw_split) {
  runSpreadCase(kSpread600Bw,
                "page_split cutter 55.7 px from the gutter centre at mid-height on the 600 dpi spread, bound 2 px "
                "(F-06-1): no requirement");
}

BOOST_AUTO_TEST_CASE(test_spread_600_color_split) {
  runSpreadCase(kSpread600Color,
                "page_split cutter 55.7 px from the gutter centre at mid-height on the 600 dpi spread, bound 2 px "
                "(F-06-1): no requirement");
}

BOOST_AUTO_TEST_SUITE_END()
