// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ExpectedFailure.h>
#include <GoldenChecks.h>
#include <PageGenerator.h>
#include <TextGolden.h>
#include <TruthMetrics.h>
#include <filters/output/DewarpingOptions.h>
#include <filters/output/Params.h>
#include <filters/page_split/LayoutType.h>

#include <QImage>
#include <QLineF>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QString>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <map>
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

// Dewarp page (D-04, D-15).  Residuals are measured in output pixels and converted to 300 dpi pixels for the bound.
const int kDewarpInputDpi = 300;
// D-15 starting value and bound: dewarped baseline residual at 300 dpi.
const double kMaxDewarpResidual300Px = 1.5;
// Noise floor of the line offset measure on a page with straight lines, output pixels at 600 dpi.  Measured 0.58 px;
// twice that is 1.16 px, rounded up to the next 0.5 px step gives 1.5 px.
const double kMaxNoiseFloorPx = 1.5;

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

test_support::PageSpec curvedSpec() {
  test_support::PageSpec spec;
  spec.kind = test_support::PageKind::CURVED;
  spec.dpi = kDewarpInputDpi;
  spec.seed = 1;
  spec.skewDeg = 0.0;
  return spec;
}

// Generated pages are shared between the scenario cases: rendering a 600 dpi page in Debug takes seconds.  Map nodes
// never move, so a reference handed out for one dpi stays valid when another page is added.
const test_support::GeneratedPage& spreadPage(const int dpi) {
  static std::map<int, test_support::GeneratedPage> cache;
  auto it = cache.find(dpi);
  if (it == cache.end()) {
    it = cache.emplace(dpi, test_support::generatePage(spreadSpec(dpi))).first;
  }
  return it->second;
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

QString intText(const long long value) {
  return QString::number(value);
}

// Stage decisions of the scenario as text golden values (D-14); the suffix of a key decides its tolerance.
test_support::TextValues spreadDecisions(const SpreadScenario& scenario, const SpreadMeasured& m) {
  using test_support::formatNumber;
  test_support::TextValues values;
  values["seed"] = intText(1);
  values["input_dpi"] = intText(scenario.dpi);
  values["applied_skew_deg"] = formatNumber(kAppliedSkewDeg);
  values["layout_type"] = intText(m.result.layoutType);
  values["cutter_count"] = intText(static_cast<long long>(m.result.cutters.size()));
  values["cutter_x1_px"] = formatNumber(m.cutter.x1());
  values["cutter_y1_px"] = formatNumber(m.cutter.y1());
  values["cutter_x2_px"] = formatNumber(m.cutter.x2());
  values["cutter_y2_px"] = formatNumber(m.cutter.y2());
  for (int i = 0; i < 2; ++i) {
    const SubPageResult& sub = m.result.pages[i];
    const QString prefix = QString::fromLatin1(sideName(i)) + "_";
    values[prefix + "deskew_angle_deg"] = formatNumber(sub.deskewAngleDeg);
    values[prefix + "content_x_px"] = intText(sub.contentRect.x());
    values[prefix + "content_y_px"] = intText(sub.contentRect.y());
    values[prefix + "content_w_px"] = intText(sub.contentRect.width());
    values[prefix + "content_h_px"] = intText(sub.contentRect.height());
    values[prefix + "page_rect_x_px"] = formatNumber(sub.pageRect.x());
    values[prefix + "page_rect_y_px"] = formatNumber(sub.pageRect.y());
    values[prefix + "page_rect_w_px"] = formatNumber(sub.pageRect.width());
    values[prefix + "page_rect_h_px"] = formatNumber(sub.pageRect.height());
    values[prefix + "hard_margin_left_mm"] = formatNumber(sub.hardMarginLeftMM);
    values[prefix + "hard_margin_top_mm"] = formatNumber(sub.hardMarginTopMM);
    values[prefix + "hard_margin_right_mm"] = formatNumber(sub.hardMarginRightMM);
    values[prefix + "hard_margin_bottom_mm"] = formatNumber(sub.hardMarginBottomMM);
    values[prefix + "output_w_px"] = intText(sub.outputSize.width());
    values[prefix + "output_h_px"] = intText(sub.outputSize.height());
    values[prefix + "output_dpi"] = intText(sub.outputDpiX);
  }
  return values;
}

void runSpreadCase(const SpreadScenario& scenario, const char* midOffsetDefect = nullptr) {
  const SpreadMeasured m = measureSpread(scenario);
  reportSpread(scenario, m);
  checkSpread(scenario, m, midOffsetDefect);

  // Goldens are recorded only when the sanity checks of both sub-pages passed (D-05).
  const test_support::ImageKind kind = scenario.colorMode == output::BLACK_AND_WHITE ? test_support::ImageKind::BILEVEL
                                                                                     : test_support::ImageKind::COLOR;
  for (int i = 0; i < 2; ++i) {
    const test_support::Sanity sanity{m.subs[i].sanity.ok, m.subs[i].sanity.reason};
    CHECK_IMAGE_GOLDEN("book", QString::fromLatin1(scenario.name) + "_" + sideName(i), m.result.pages[i].output, kind,
                       sanity);
  }
  const bool bothOk = m.subs[0].sanity.ok && m.subs[1].sanity.ok;
  const QString reason = !m.subs[0].sanity.ok
                             ? "left sub-page: " + m.subs[0].sanity.reason
                             : (!m.subs[1].sanity.ok ? "right sub-page: " + m.subs[1].sanity.reason : QString());
  CHECK_TEXT_GOLDEN("book", scenario.name, spreadDecisions(scenario, m), (test_support::Sanity{bothOk, reason}));
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

// Fingerprints of every generated book input, so that a change of the generator shows up as a fixture change and not
// as a pipeline change.
BOOST_AUTO_TEST_CASE(test_book_fixture_fingerprints) {
  test_support::TextValues values;
  values["spread_300"] = QString::fromLatin1(test_support::fingerprint(spreadPage(300).image));
  values["spread_600"] = QString::fromLatin1(test_support::fingerprint(spreadPage(600).image));
  values["curved_300"] = QString::fromLatin1(test_support::fingerprint(test_support::generatePage(curvedSpec()).image));
  BOOST_REQUIRE_EQUAL(values.size(), 3u);
  CHECK_TEXT_GOLDEN("book", "fixtures", values, (test_support::Sanity{true, QString()}));
}

// The cache must hand out references that stay valid while it grows.  The case adds at least 16 low-dpi pages that no
// other case renders, so a container that reallocates when it grows must move the first page inside this case,
// whatever ran before.  Only addresses are compared before the check: a reallocating container would leave the first
// pointer dangling.
BOOST_AUTO_TEST_CASE(test_spread_page_cache_keeps_references) {
  const test_support::GeneratedPage* const first = &spreadPage(75);
  int added = 0;
  for (int dpi = 105; dpi < 150 && added < 17; ++dpi) {
    if (test_support::dpiSupported(test_support::PageKind::SPREAD, dpi)) {
      spreadPage(dpi);
      ++added;
    }
  }
  BOOST_REQUIRE_GE(added, 16);
  BOOST_REQUIRE_MESSAGE(&spreadPage(75) == first,
                        "the 75 dpi spread page moved when " << added << " more pages were added to the cache");
  BOOST_CHECK(!first->image.isNull());
}

BOOST_AUTO_TEST_SUITE_END()


namespace {

// Line offsets against the truth mask (the straightness measure of the dewarp cases).
//
// TruthMetrics::straightnessResidual fits the ink-weighted centre row of each text line band in vertical strips.  On
// the 5x7 dot font that centre depends on which glyphs sit in the strip (the dots of i and j split a band, descenders
// and ascenders shift the centre by several pixels), so it has a noise floor of about 4 to 8 px at 600 dpi on this page
// and, with 8 strips, finds no usable strip at all in the output without dewarping (records/scenario-book.md,
// "Dewarp measurements").  This measure removes the glyph content from the comparison: for every line and strip it
// finds the vertical shift of the truth mask's row profile (the same glyphs, flat, at the output dpi) that best matches
// the output's row profile, and the residual is how far those shifts of one line stray from a straight line over the
// strips.  A straight, tilted line has residual 0.
const int kBaseDotPx = 5;  // one glyph dot in base pixels (1/300 in), PageGenerator.h
const int kGlyphRows = 7;
const int kTemplatePadPx = 12;
const int kGlobalSearchPx = 100;
const int kFirstStripSearchPx = 54;  // less than half the line pitch of 11 dots
const int kTrackingSearchPx = 30;
const int kStraightnessStrips = 8;

struct LineOffsets {
  bool valid = false;
  double maxResidualPx = 0.0;
  int worstLine = -1;
  int worstStrip = -1;
  std::vector<std::vector<int>> offsets;  // [line][strip]: shift of the truth line, output rows
};

std::vector<int> rowProfile(const QImage& gray, const int x0, const int x1) {
  std::vector<int> profile(static_cast<size_t>(gray.height()), 0);
  const int left = std::max(0, x0);
  const int right = std::min(gray.width(), x1);
  for (int y = 0; y < gray.height(); ++y) {
    const uchar* const line = gray.constScanLine(y);
    int count = 0;
    for (int x = left; x < right; ++x) {
      count += line[x] < 128 ? 1 : 0;
    }
    profile[static_cast<size_t>(y)] = count;
  }
  return profile;
}

// Sum of squared differences between the template and the output profile starting at row y0 (rows outside the image
// count as empty).
long long profileDistance(const std::vector<int>& outProfile, const int y0, const std::vector<int>& tmpl) {
  long long sum = 0;
  const int height = static_cast<int>(outProfile.size());
  for (size_t r = 0; r < tmpl.size(); ++r) {
    const int y = y0 + static_cast<int>(r);
    const long long value = (y >= 0 && y < height) ? outProfile[static_cast<size_t>(y)] : 0;
    const long long diff = value - tmpl[r];
    sum += diff * diff;
  }
  return sum;
}

// truthLineTops are the first rows of the glyph cells of the truth mask, dotPx the size of one glyph dot at the mask's
// dpi.
LineOffsets measureLineOffsets(const QImage& output,
                               const QImage& truthMask,
                               const std::vector<int>& truthLineTops,
                               const int dotPx,
                               const int strips) {
  LineOffsets result;
  const QImage outGray = output.convertToFormat(QImage::Format_Grayscale8);
  const QRect outBox = test_support::nonBackgroundBox(outGray);
  const QRect maskBox = test_support::nonBackgroundBox(truthMask);
  if (outBox.isNull() || maskBox.isNull() || truthLineTops.empty() || strips < 3) {
    return result;
  }
  const int lines = static_cast<int>(truthLineTops.size());
  const int cellHeight = kGlyphRows * dotPx;
  const int templateRows = cellHeight + 2 * kTemplatePadPx;
  const int rowShift = outBox.top() - maskBox.top();

  std::vector<std::vector<std::vector<int>>> templates(static_cast<size_t>(lines));  // [line][strip][row]
  std::vector<std::vector<int>> outProfiles;                                         // [strip][row]
  for (int s = 0; s < strips; ++s) {
    const int mx0 = maskBox.left() + maskBox.width() * s / strips;
    const int mx1 = maskBox.left() + maskBox.width() * (s + 1) / strips;
    const std::vector<int> maskProfile = rowProfile(truthMask, mx0, mx1);
    for (int k = 0; k < lines; ++k) {
      const int top = truthLineTops[static_cast<size_t>(k)] - kTemplatePadPx;
      templates[static_cast<size_t>(k)].emplace_back(maskProfile.begin() + top,
                                                     maskProfile.begin() + top + templateRows);
    }
    const int ox0 = outBox.left() + outBox.width() * s / strips;
    const int ox1 = outBox.left() + outBox.width() * (s + 1) / strips;
    outProfiles.push_back(rowProfile(outGray, ox0, ox1));
  }
  auto nominalRow = [&](const int k) { return truthLineTops[static_cast<size_t>(k)] - kTemplatePadPx + rowShift; };

  // The first strip is the far edge of the page, where the curl is zero: find the one shift of all lines together.
  int globalShift = 0;
  long long bestTotal = -1;
  for (int dy = -kGlobalSearchPx; dy <= kGlobalSearchPx; ++dy) {
    long long total = 0;
    for (int k = 0; k < lines; ++k) {
      total += profileDistance(outProfiles[0], nominalRow(k) + dy, templates[static_cast<size_t>(k)][0]);
    }
    if (bestTotal < 0 || total < bestTotal) {
      bestTotal = total;
      globalShift = dy;
    }
  }

  result.offsets.assign(static_cast<size_t>(lines), std::vector<int>(static_cast<size_t>(strips), 0));
  for (int k = 0; k < lines; ++k) {
    int previous = globalShift;
    for (int s = 0; s < strips; ++s) {
      const int radius = s == 0 ? kFirstStripSearchPx : kTrackingSearchPx;
      int best = previous;
      long long bestDistance = -1;
      for (int dy = previous - radius; dy <= previous + radius; ++dy) {
        const long long distance = profileDistance(outProfiles[static_cast<size_t>(s)], nominalRow(k) + dy,
                                                   templates[static_cast<size_t>(k)][static_cast<size_t>(s)]);
        // A tie goes to the shift nearest the previous strip.
        if (bestDistance < 0 || distance < bestDistance
            || (distance == bestDistance && std::abs(dy - previous) < std::abs(best - previous))) {
          bestDistance = distance;
          best = dy;
        }
      }
      result.offsets[static_cast<size_t>(k)][static_cast<size_t>(s)] = best;
      previous = best;
    }
  }

  // Least-squares line over the strip index for every line, largest absolute deviation.
  for (int k = 0; k < lines; ++k) {
    const std::vector<int>& d = result.offsets[static_cast<size_t>(k)];
    double sumX = 0, sumY = 0, sumXX = 0, sumXY = 0;
    for (int s = 0; s < strips; ++s) {
      sumX += s;
      sumY += d[static_cast<size_t>(s)];
      sumXX += static_cast<double>(s) * s;
      sumXY += static_cast<double>(s) * d[static_cast<size_t>(s)];
    }
    const double n = strips;
    const double slope = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
    const double intercept = (sumY - slope * sumX) / n;
    for (int s = 0; s < strips; ++s) {
      const double residual = std::fabs(d[static_cast<size_t>(s)] - (intercept + slope * s));
      if (residual > result.maxResidualPx) {
        result.maxResidualPx = residual;
        result.worstLine = k;
        result.worstStrip = s;
      }
    }
  }
  result.valid = true;
  return result;
}

struct DewarpRun {
  PipelineResult result;
  QImage output;
  LineOffsets offsets;
  test_support::StraightnessResult planStraightness;  // straightnessResidual with 8 strips, reported for reference
};

DewarpRun runCurved(const test_support::GeneratedPage& page, const output::DewarpingMode mode) {
  PipelineOptions options;
  options.layout = page_split::SINGLE_PAGE_UNCUT;
  options.colorMode = output::BLACK_AND_WHITE;
  options.dewarping = mode;
  DewarpRun run;
  run.result = runPipeline(page.image, options);
  BOOST_REQUIRE_EQUAL(run.result.pages.size(), 1u);
  run.output = run.result.pages.front().output;
  BOOST_REQUIRE_MESSAGE(!run.output.isNull(), "the curved page produced no output image");

  const int scale = kOutputDpi / kDewarpInputDpi;
  std::vector<int> truthTops;
  for (const int top : page.truth.lineTops) {
    truthTops.push_back(top * scale);
  }
  const QImage mask = test_support::renderInkMask(curvedSpec(), kOutputDpi, 0.0);
  run.offsets = measureLineOffsets(run.output, mask, truthTops, kBaseDotPx * scale, kStraightnessStrips);
  run.planStraightness = test_support::straightnessResidual(run.output, test_support::nonBackgroundBox(run.output),
                                                            page.truth.lineCount, kStraightnessStrips);
  return run;
}

void reportDewarpRun(const std::string& scenario, const char* label, const DewarpRun& run) {
  const std::string prefix = label;
  reportMetric(scenario, (prefix + "_line_offset_residual_px").c_str(), run.offsets.maxResidualPx, "none");
  reportMetric(scenario, (prefix + "_worst_line").c_str(), run.offsets.worstLine, "none");
  reportMetric(scenario, (prefix + "_worst_strip").c_str(), run.offsets.worstStrip, "none");
  reportMetric(scenario, (prefix + "_plan_straightness_valid").c_str(), run.planStraightness.valid ? 1 : 0, "none");
  reportMetric(scenario, (prefix + "_plan_straightness_strips_used").c_str(), run.planStraightness.stripsUsed, "none");
  if (run.planStraightness.valid) {
    reportMetric(scenario, (prefix + "_plan_straightness_residual_px").c_str(), run.planStraightness.maxResidualPx,
                 "none");
  }
}

}  // namespace

BOOST_AUTO_TEST_SUITE(DewarpPageTestSuite)

BOOST_AUTO_TEST_CASE(test_curved_300_auto_dewarp_straightens) {
  const std::string scenario = "curved_300_dewarp";
  const test_support::GeneratedPage page = test_support::generatePage(curvedSpec());
  BOOST_REQUIRE_MESSAGE(!page.image.isNull(), "the generator returned a null curved page");

  const DewarpRun off = runCurved(page, output::OFF);
  const DewarpRun autoRun = runCurved(page, output::AUTO);
  const SubPageResult& autoSub = autoRun.result.pages.front();
  const bool modelValid = autoRun.result.settings.output->getParams(autoSub.pageId).distortionModel().isValid();

  BOOST_REQUIRE_MESSAGE(off.offsets.valid, scenario << ": no line offsets without dewarping");
  BOOST_REQUIRE_MESSAGE(autoRun.offsets.valid, scenario << ": no line offsets with AUTO dewarping");

  const double residualOff = off.offsets.maxResidualPx;
  const double residualAuto = autoRun.offsets.maxResidualPx;
  const double residualAuto300 = residualAuto * kDewarpInputDpi / autoSub.outputDpiX;
  reportMetric(scenario, "residual_off_px", residualOff, "none");
  reportMetric(scenario, "residual_auto_px", residualAuto, num(residualOff));
  reportMetric(scenario, "residual_auto_300dpi_px", residualAuto300, num(kMaxDewarpResidual300Px));
  reportMetric(scenario, "residual_ratio_auto_over_off", residualOff > 0.0 ? residualAuto / residualOff : -1.0, "<1");
  reportDewarpRun(scenario, "off", off);
  reportDewarpRun(scenario, "auto", autoRun);
  reportMetric(scenario, "distortion_model_valid", modelValid ? 1 : 0, "none");
  reportMetric(scenario, "output_w_px", autoSub.outputSize.width(), "none");
  reportMetric(scenario, "output_h_px", autoSub.outputSize.height(), "none");
  reportMetric(scenario, "output_off_w_px", off.result.pages.front().outputSize.width(), "none");
  reportMetric(scenario, "output_off_h_px", off.result.pages.front().outputSize.height(), "none");

  // D-04: dewarped text lines are straighter than the same page without dewarping.
  BOOST_CHECK_MESSAGE(residualAuto < residualOff, scenario << ": AUTO dewarping residual " << num(residualAuto)
                                                           << " px is not below the residual without dewarping "
                                                           << num(residualOff) << " px");
  // D-15 bound.
  BOOST_CHECK_MESSAGE(residualAuto300 <= kMaxDewarpResidual300Px,
                      scenario << ": AUTO dewarping leaves " << num(residualAuto300)
                               << " px residual at 300 dpi, bound " << num(kMaxDewarpResidual300Px) << " px");

  // Golden of the AUTO output, recorded only when the straightness measure found every line and the ink fraction is
  // plausible (D-05).
  const double ink = test_support::inkFraction(autoSub.output);
  QString reason;
  if (!off.offsets.valid || !autoRun.offsets.valid) {
    reason = "no valid straightness result";
  } else if (ink < kMinInkFractionOfOutput || ink > kMaxInkFractionOfOutput) {
    reason = QString("ink fraction %1 is outside [0.005, 0.5]").arg(ink);
  }
  const test_support::Sanity sanity{reason.isEmpty(), reason};
  CHECK_IMAGE_GOLDEN("book", QString::fromStdString(scenario), autoSub.output, test_support::ImageKind::BILEVEL,
                     sanity);

  using test_support::formatNumber;
  test_support::TextValues values;
  values["seed"] = QString::number(1);
  values["input_dpi"] = QString::number(kDewarpInputDpi);
  values["residual_off_px"] = formatNumber(residualOff);
  values["residual_auto_px"] = formatNumber(residualAuto);
  values["distortion_model_valid"] = QString::number(modelValid ? 1 : 0);
  values["output_w_px"] = QString::number(autoSub.outputSize.width());
  values["output_h_px"] = QString::number(autoSub.outputSize.height());
  values["output_dpi"] = QString::number(autoSub.outputDpiX);
  CHECK_TEXT_GOLDEN("book", QString::fromStdString(scenario), values, sanity);
}

// Guard for the measure above: a page whose lines are straight must measure (almost) zero, so the residuals of the
// curved page are not glyph-content noise.
BOOST_AUTO_TEST_CASE(test_line_offset_measure_noise_floor_on_flat_page) {
  const std::string scenario = "flat_300_noise_floor";
  test_support::PageSpec spec;
  spec.kind = test_support::PageKind::FLAT;
  spec.dpi = kDewarpInputDpi;
  spec.seed = 1;
  spec.skewDeg = 0.0;
  const test_support::GeneratedPage page = test_support::generatePage(spec);
  BOOST_REQUIRE_MESSAGE(!page.image.isNull(), "the generator returned a null flat page");
  PipelineOptions options;
  const PipelineResult result = runPipeline(page.image, options);
  BOOST_REQUIRE_EQUAL(result.pages.size(), 1u);

  const int scale = kOutputDpi / kDewarpInputDpi;
  std::vector<int> truthTops;
  for (const int top : page.truth.lineTops) {
    truthTops.push_back(top * scale);
  }
  const QImage mask = test_support::renderInkMask(spec, kOutputDpi, 0.0);
  const LineOffsets offsets
      = measureLineOffsets(result.pages.front().output, mask, truthTops, kBaseDotPx * scale, kStraightnessStrips);
  BOOST_REQUIRE_MESSAGE(offsets.valid, scenario << ": no line offsets");
  reportMetric(scenario, "line_offset_residual_px", offsets.maxResidualPx, num(kMaxNoiseFloorPx));
  BOOST_CHECK_MESSAGE(offsets.maxResidualPx <= kMaxNoiseFloorPx, scenario << ": a straight page measures "
                                                                          << num(offsets.maxResidualPx) << " px, limit "
                                                                          << num(kMaxNoiseFloorPx));
}

BOOST_AUTO_TEST_SUITE_END()
