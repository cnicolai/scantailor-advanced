// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ExpectedFailure.h>
#include <GoldenChecks.h>
#include <PageGenerator.h>
#include <TextGolden.h>
#include <TruthMetrics.h>

#include <QImage>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QString>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "HeadlessPipeline.h"

namespace {

// Ground-truth bounds.  Starting values of D-15, then set by the D-15 tightening rule from the measurements in
// records/scenario-flat.md ("Tolerance decisions").
const double kAngleToleranceDeg = 0.15;
const double kMinContentIou = 0.96;  // worst 1 - IoU 0.0187, twice rounded up to 0.005: 0.04
const double kMaxBinarizationMismatch = 0.005;
// Colour/greyscale output: mean grey under the truth ink must be at least this many levels below the mean grey
// under the truth paper.  Not a D-15 value (D-15 names no colour check), so it is not tightened.
const double kMinInkPaperContrast = 100.0;

const double kAppliedSkewDeg = 1.5;
const int kOutputDpi = 600;
const int kRegistrationRadius = 4;
// Sanity (D-05): the glyph ink starts within this distance of the stored hard margins, at 600 dpi.
const double kMarginSlackMm = 5.0;
const double kMillimetresPerInch = 25.4;
const double kSanityWidthSlack = 1.2;
const double kMinInkFractionOfOutput = 0.005;
const double kMaxInkFractionOfOutput = 0.5;

struct Scenario {
  const char* name;
  int dpi;
  output::ColorMode colorMode;
};

const Scenario kScenario300Bw = {"flat_300_bw", 300, output::BLACK_AND_WHITE};
const Scenario kScenario300Color = {"flat_300_color", 300, output::COLOR_GRAYSCALE};
const Scenario kScenario600Bw = {"flat_600_bw", 600, output::BLACK_AND_WHITE};
const Scenario kScenario600Color = {"flat_600_color", 600, output::COLOR_GRAYSCALE};

const double kSweepAnglesDeg[] = {-3.0, -1.5, -0.5, 0.5, 1.5, 2.0, 3.0};
const int kSweepDpis[] = {300, 600};

std::string num(const double value) {
  return QString::number(value, 'g', 10).toStdString();
}

void reportMetric(const std::string& scenario, const char* metric, const double value, const std::string& bound) {
  BOOST_TEST_MESSAGE("TRUTH-METRIC suite=flat scenario=" << scenario << " metric=" << metric << " value=" << num(value)
                                                         << " bound=" << bound);
}

test_support::PageSpec flatSpec(const int dpi, const double skewDeg) {
  test_support::PageSpec spec;
  spec.kind = test_support::PageKind::FLAT;
  spec.dpi = dpi;
  spec.seed = 1;
  spec.skewDeg = skewDeg;
  return spec;
}

// Generated pages are shared between the scenario cases, the sweep and the fingerprint case: rendering a 600 dpi
// page in Debug takes seconds.
const test_support::GeneratedPage& flatPage(const int dpi, const double skewDeg) {
  static std::map<std::pair<int, int>, test_support::GeneratedPage> cache;
  const std::pair<int, int> key(dpi, static_cast<int>(std::lround(skewDeg * 10.0)));
  auto it = cache.find(key);
  if (it == cache.end()) {
    it = cache.emplace(key, test_support::generatePage(flatSpec(dpi, skewDeg))).first;
  }
  return it->second;
}

// Angle as used in golden keys: sign letter (p or m), integer part, "p", first decimal: 1.5 is p1p5, -3.0 is m3p0.
std::string angleLabel(const double angleDeg) {
  const long tenths = std::lround(std::fabs(angleDeg) * 10.0);
  return std::string(angleDeg < 0 ? "m" : "p") + std::to_string(tenths / 10) + "p" + std::to_string(tenths % 10);
}

struct SanityResult {
  bool ok;
  QString reason;
};

// Everything measured for one scenario.
struct Measured {
  test_support::GeneratedPage page;
  PipelineResult result;
  double deskewErrorDeg = 0.0;
  double contentIou = 0.0;
  double binarizationMismatch = -1.0;  // black-and-white scenarios only
  double inkPaperContrast = -1.0;      // colour scenarios only (paper grey minus ink grey)
  QPoint registrationOffset;
  SanityResult sanity{false, QString()};
};

double meanGreyUnder(const QImage& output, const QImage& mask, const QPoint& offset, const bool ink) {
  const QImage gray = output.convertToFormat(QImage::Format_Grayscale8);
  double sum = 0.0;
  long long count = 0;
  for (int y = 0; y < mask.height(); ++y) {
    const uchar* const maskLine = mask.constScanLine(y);
    const int oy = y + offset.y();
    if (oy < 0 || oy >= gray.height()) {
      continue;
    }
    const uchar* const outLine = gray.constScanLine(oy);
    for (int x = 0; x < mask.width(); ++x) {
      const int ox = x + offset.x();
      if (ox < 0 || ox >= gray.width()) {
        continue;
      }
      if ((maskLine[x] < 128) == ink) {
        sum += outLine[ox];
        ++count;
      }
    }
  }
  return count > 0 ? sum / static_cast<double>(count) : 0.0;
}

SanityResult flatSanity(const Scenario& scenario, const test_support::GeneratedPage& page, const SubPageResult& sub) {
  const QImage& out = sub.output;
  const double scale = static_cast<double>(kOutputDpi) / scenario.dpi;
  test_support::SanityExpectation expectation;
  expectation.minSize = QSize(static_cast<int>(scale * page.truth.textBlock.width()),
                              static_cast<int>(scale * page.truth.textBlock.height()));
  expectation.maxSize = QSize(static_cast<int>(scale * page.truth.pageSize.width() * kSanityWidthSlack), 0);
  expectation.minInk = kMinInkFractionOfOutput;
  expectation.maxInk = kMaxInkFractionOfOutput;
  // The default hard margins (10 mm left and right, 5 mm top and bottom) make the whole page of this fixture
  // nearly square (1676 x 1636 px), so the orientation is checked on the glyph ink box below, not on the page.
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

Measured measureScenario(const Scenario& scenario) {
  Measured m;
  m.page = flatPage(scenario.dpi, kAppliedSkewDeg);
  PipelineOptions options;
  options.colorMode = scenario.colorMode;
  m.result = runPipeline(m.page.image, options);
  BOOST_REQUIRE_EQUAL(m.result.pages.size(), 1u);
  const SubPageResult& sub = m.result.pages.front();
  BOOST_REQUIRE_MESSAGE(!sub.output.isNull(), scenario.name << ": no output image");
  BOOST_REQUIRE_MESSAGE(sub.pageRect.isValid() && sub.contentRect.isValid(),
                        scenario.name << ": select_content found no boxes");

  // The stored angle straightens the page, so it has the opposite sign of the angle the generator applied
  // (PageGenerator.h).
  m.deskewErrorDeg = std::fabs(sub.deskewAngleDeg + kAppliedSkewDeg);

  const QPointF truthCentre(m.page.truth.pageSize.width() / 2.0, m.page.truth.pageSize.height() / 2.0);
  m.contentIou = test_support::centredBoxIoU(QRectF(sub.contentRect), sub.pageRect.center(),
                                             QRectF(m.page.truth.textBlock), truthCentre);

  // Rotation left on the page after the stored correction, in the generator's convention (positive turns the
  // content clockwise).
  const double netRotationDeg = kAppliedSkewDeg + sub.deskewAngleDeg;
  const QImage mask = test_support::renderInkMask(flatSpec(scenario.dpi, kAppliedSkewDeg), kOutputDpi, netRotationDeg);
  const test_support::Registration registration = test_support::registerMask(sub.output, mask, kRegistrationRadius);
  m.registrationOffset = registration.offset;
  const int edgeRadius = kOutputDpi / scenario.dpi;
  if (scenario.colorMode == output::BLACK_AND_WHITE) {
    m.binarizationMismatch = test_support::truthMismatchFraction(sub.output, mask, registration.offset, edgeRadius);
  } else {
    m.inkPaperContrast = meanGreyUnder(sub.output, mask, registration.offset, false)
                         - meanGreyUnder(sub.output, mask, registration.offset, true);
  }
  m.sanity = flatSanity(scenario, m.page, sub);
  return m;
}

void reportScenario(const Scenario& scenario, const Measured& m) {
  const SubPageResult& sub = m.result.pages.front();
  const std::string name = scenario.name;
  reportMetric(name, "stored_deskew_deg", sub.deskewAngleDeg, "none");
  reportMetric(name, "deskew_error_deg", m.deskewErrorDeg, num(kAngleToleranceDeg));
  reportMetric(name, "content_iou", m.contentIou, num(kMinContentIou));
  if (m.binarizationMismatch >= 0.0) {
    reportMetric(name, "binarization_mismatch", m.binarizationMismatch, num(kMaxBinarizationMismatch));
  } else {
    reportMetric(name, "ink_paper_contrast", m.inkPaperContrast, num(kMinInkPaperContrast));
  }
  reportMetric(name, "output_w_px", sub.outputSize.width(), "none");
  reportMetric(name, "output_h_px", sub.outputSize.height(), "none");
  reportMetric(name, "hard_margin_left_mm", sub.hardMarginLeftMM, "none");
  reportMetric(name, "hard_margin_top_mm", sub.hardMarginTopMM, "none");
  reportMetric(name, "hard_margin_right_mm", sub.hardMarginRightMM, "none");
  reportMetric(name, "hard_margin_bottom_mm", sub.hardMarginBottomMM, "none");
  reportMetric(name, "content_w_px", sub.contentRect.width(), "none");
  reportMetric(name, "content_h_px", sub.contentRect.height(), "none");
  reportMetric(name, "page_rect_w_px", sub.pageRect.width(), "none");
  reportMetric(name, "page_rect_h_px", sub.pageRect.height(), "none");
  reportMetric(name, "registration_offset_x_px", m.registrationOffset.x(), "none");
  reportMetric(name, "registration_offset_y_px", m.registrationOffset.y(), "none");
  BOOST_TEST_MESSAGE("TRUTH-METRIC suite=flat scenario=" << name << " metric=sanity value=" << (m.sanity.ok ? 1 : 0)
                                                         << " bound=1 reason=" << m.sanity.reason.toStdString());
}

// knownDefect: when not null, the binarization check is an expected failure (D-18) with this tag.
void checkScenario(const Scenario& scenario, const Measured& m, const char* knownDefect) {
  const std::string name = scenario.name;
  BOOST_CHECK_MESSAGE(m.deskewErrorDeg <= kAngleToleranceDeg,
                      name << ": deskew error " << num(m.deskewErrorDeg) << " deg exceeds " << num(kAngleToleranceDeg));
  BOOST_CHECK_MESSAGE(m.contentIou >= kMinContentIou,
                      name << ": content box IoU " << num(m.contentIou) << " is below " << num(kMinContentIou));
  if (scenario.colorMode == output::BLACK_AND_WHITE) {
    const bool binarizationOk = m.binarizationMismatch >= 0.0 && m.binarizationMismatch <= kMaxBinarizationMismatch;
    if (knownDefect != nullptr) {
      XFAIL_CHECK(binarizationOk, knownDefect);
    } else {
      BOOST_CHECK_MESSAGE(binarizationOk, name << ": binarization mismatch " << num(m.binarizationMismatch)
                                               << " exceeds " << num(kMaxBinarizationMismatch));
    }
  } else {
    BOOST_CHECK_MESSAGE(m.inkPaperContrast >= kMinInkPaperContrast, name << ": ink/paper contrast "
                                                                         << num(m.inkPaperContrast) << " is below "
                                                                         << num(kMinInkPaperContrast));
  }
  const SubPageResult& sub = m.result.pages.front();
  BOOST_CHECK_EQUAL(sub.outputDpiX, kOutputDpi);
  BOOST_CHECK_EQUAL(sub.outputDpiY, kOutputDpi);
}

QString intText(const long long value) {
  return QString::number(value);
}

// Stage decisions of the scenario as text golden values (D-14); the suffix of a key decides its tolerance.
test_support::TextValues stageDecisions(const Scenario& scenario, const Measured& m) {
  using test_support::formatNumber;
  const SubPageResult& sub = m.result.pages.front();
  test_support::TextValues values;
  values["seed"] = intText(1);
  values["input_dpi"] = intText(scenario.dpi);
  values["applied_skew_deg"] = formatNumber(kAppliedSkewDeg);
  values["deskew_angle_deg"] = formatNumber(sub.deskewAngleDeg);
  values["layout_type"] = intText(m.result.layoutType);
  values["content_x_px"] = intText(sub.contentRect.x());
  values["content_y_px"] = intText(sub.contentRect.y());
  values["content_w_px"] = intText(sub.contentRect.width());
  values["content_h_px"] = intText(sub.contentRect.height());
  values["page_rect_x_px"] = formatNumber(sub.pageRect.x());
  values["page_rect_y_px"] = formatNumber(sub.pageRect.y());
  values["page_rect_w_px"] = formatNumber(sub.pageRect.width());
  values["page_rect_h_px"] = formatNumber(sub.pageRect.height());
  values["hard_margin_left_mm"] = formatNumber(sub.hardMarginLeftMM);
  values["hard_margin_top_mm"] = formatNumber(sub.hardMarginTopMM);
  values["hard_margin_right_mm"] = formatNumber(sub.hardMarginRightMM);
  values["hard_margin_bottom_mm"] = formatNumber(sub.hardMarginBottomMM);
  values["output_w_px"] = intText(sub.outputSize.width());
  values["output_h_px"] = intText(sub.outputSize.height());
  values["output_dpi"] = intText(sub.outputDpiX);
  return values;
}

void runScenarioCase(const Scenario& scenario, const char* knownDefect = nullptr) {
  const Measured m = measureScenario(scenario);
  reportScenario(scenario, m);
  checkScenario(scenario, m, knownDefect);

  // Goldens are recorded only when the sanity checks of the scenario passed (D-05).
  const test_support::Sanity sanity{m.sanity.ok, m.sanity.reason};
  const test_support::ImageKind kind = scenario.colorMode == output::BLACK_AND_WHITE ? test_support::ImageKind::BILEVEL
                                                                                     : test_support::ImageKind::COLOR;
  CHECK_IMAGE_GOLDEN("flat", scenario.name, m.result.pages.front().output, kind, sanity);
  CHECK_TEXT_GOLDEN("flat", scenario.name, stageDecisions(scenario, m), sanity);
}

}  // namespace

BOOST_AUTO_TEST_SUITE(FlatDocumentTestSuite)

// Tracer case of plan 02-01: stage decisions of the 300 dpi black-and-white page against ground truth.
BOOST_AUTO_TEST_CASE(test_flat_300_bw_deskew_and_content_box) {
  const test_support::GeneratedPage& page = flatPage(300, kAppliedSkewDeg);
  const PipelineResult result = runPipeline(page.image, PipelineOptions());
  BOOST_REQUIRE_EQUAL(result.pages.size(), 1u);
  const SubPageResult& sub = result.pages.front();

  const double angleError = std::fabs(sub.deskewAngleDeg + kAppliedSkewDeg);
  BOOST_CHECK_MESSAGE(angleError <= kAngleToleranceDeg,
                      "deskew angle error " << num(angleError) << " exceeds " << num(kAngleToleranceDeg));

  BOOST_REQUIRE_MESSAGE(sub.pageRect.isValid() && sub.contentRect.isValid(), "select_content found no boxes");
  const QPointF truthCentre(page.truth.pageSize.width() / 2.0, page.truth.pageSize.height() / 2.0);
  const double iou = test_support::centredBoxIoU(QRectF(sub.contentRect), sub.pageRect.center(),
                                                 QRectF(page.truth.textBlock), truthCentre);
  BOOST_CHECK_MESSAGE(iou >= kMinContentIou, "content box IoU " << num(iou) << " is below " << num(kMinContentIou));

  BOOST_REQUIRE_MESSAGE(!sub.output.isNull(), "no output image");
  const double ink = test_support::inkFraction(sub.output);
  BOOST_CHECK_MESSAGE(ink >= kMinInkFractionOfOutput && ink <= kMaxInkFractionOfOutput,
                      "output black fraction " << num(ink) << " is outside [" << num(kMinInkFractionOfOutput) << ", "
                                               << num(kMaxInkFractionOfOutput) << "]");

  BOOST_CHECK_EQUAL(sub.outputDpiX, kOutputDpi);
  BOOST_CHECK_EQUAL(sub.outputDpiY, kOutputDpi);
  BOOST_CHECK_MESSAGE(sub.outputSize.width() >= 2.0 * page.truth.textBlock.width(),
                      "output width " << sub.outputSize.width() << " is smaller than twice the text block width "
                                      << page.truth.textBlock.width());
}

BOOST_AUTO_TEST_CASE(test_flat_300_bw_truth) {
  runScenarioCase(kScenario300Bw);
}

BOOST_AUTO_TEST_CASE(test_flat_300_color_truth) {
  runScenarioCase(kScenario300Color);
}

BOOST_AUTO_TEST_CASE(test_flat_600_bw_truth) {
  // F-05-1: the unmodified output stage leaves about 0.1 degrees of residual rotation on a 600 dpi input although
  // the stored deskew angle is exact.
  runScenarioCase(kScenario600Bw,
                  "output of the 600 dpi flat page keeps about 0.1 deg residual skew, binarization mismatch 8.3% "
                  "against 0.5% (F-05-1): no requirement");
}

BOOST_AUTO_TEST_CASE(test_flat_600_color_truth) {
  runScenarioCase(kScenario600Color);
}

// Seven applied angles at both DPIs, stopping after the deskew stage.
BOOST_AUTO_TEST_CASE(test_deskew_angle_sweep) {
  for (const int dpi : kSweepDpis) {
    for (const double appliedDeg : kSweepAnglesDeg) {
      const test_support::GeneratedPage& page = flatPage(dpi, appliedDeg);
      PipelineOptions options;
      options.lastStage = LastStage::DESKEW;
      const PipelineResult result = runPipeline(page.image, options);
      BOOST_REQUIRE_EQUAL(result.pages.size(), 1u);
      const double storedDeg = result.pages.front().deskewAngleDeg;
      const double errorDeg = std::fabs(storedDeg + appliedDeg);
      const std::string scenario = "sweep_" + std::to_string(dpi) + "_" + angleLabel(appliedDeg);
      reportMetric(scenario, "applied_deg", appliedDeg, "none");
      reportMetric(scenario, "stored_deg", storedDeg, "none");
      reportMetric(scenario, "deskew_error_deg", errorDeg, num(kAngleToleranceDeg));
      BOOST_CHECK_MESSAGE(errorDeg <= kAngleToleranceDeg,
                          scenario << ": deskew error " << num(errorDeg) << " deg exceeds " << num(kAngleToleranceDeg));
    }
  }
}

// Fingerprints of every generated flat input, so that a change of the generator shows up as a fixture change and not
// as a pipeline change.  The four scenario keys repeat the fingerprint of the 1.5 degree sweep input they use.
BOOST_AUTO_TEST_CASE(test_flat_fixture_fingerprints) {
  test_support::TextValues values;
  for (const int dpi : kSweepDpis) {
    for (const double appliedDeg : kSweepAnglesDeg) {
      const std::string key = "flat_" + std::to_string(dpi) + "_skew_" + angleLabel(appliedDeg);
      values[QString::fromStdString(key)]
          = QString::fromLatin1(test_support::fingerprint(flatPage(dpi, appliedDeg).image));
    }
  }
  for (const Scenario* scenario : {&kScenario300Bw, &kScenario300Color, &kScenario600Bw, &kScenario600Color}) {
    values[QString("scenario_") + scenario->name]
        = QString::fromLatin1(test_support::fingerprint(flatPage(scenario->dpi, kAppliedSkewDeg).image));
  }
  BOOST_REQUIRE_EQUAL(values.size(), 18u);
  CHECK_TEXT_GOLDEN("flat", "fixtures", values, (test_support::Sanity{true, QString()}));
}

BOOST_AUTO_TEST_SUITE_END()
