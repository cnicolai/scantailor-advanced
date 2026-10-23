// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ExpectedFailure.h>
#include <GoldenChecks.h>
#include <PageGenerator.h>
#include <TextGolden.h>
#include <TruthMetrics.h>
#include <filters/page_split/LayoutType.h>

#include <QImage>
#include <QLineF>
#include <QPoint>
#include <QRect>
#include <QString>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "HeadlessPipeline.h"

namespace {

// Ground-truth bounds.  Starting values of plan 02-07, then set by the D-15 tightening rule from the measurements in
// records/scenario-mixed.md ("Tolerance decisions").
// Share of the specks present with despeckling off that the default removes.  Start 0.90; worst case 1 (shortfall 0),
// so one step of 0.05 percentage points.
const double kMinSpeckRemovedShare = 0.9995;
// Relative change of the glyph component count between the two runs.  Start 1%; worst case 0, so one step.
const double kMaxGlyphComponentChange = 0.0005;
// Fixture sanity: with despeckling off at least this share of the generator's specks inside the content box must show
// in the output, and at least kMinReachableSpecks specks must lie there, else the page cannot test despeckling.
const double kMinSpecksPresentOff = 0.90;
const int kMinReachableSpecks = 10;

const int kOutputDpi = 600;
const int kRegistrationRadius = 4;
const int kSpeckGrowPx = 1;               // a speck is present when its mapped rectangle grown by this holds ink
const int kTextBlockGrowPx = 4;           // glyph components are counted in the mapped text block grown by this
const int kPictureShrinkPx = 4;           // picture checks look inside the mapped picture rectangle shrunk by this
const int kPictureBarWidthPx = 3;         // see registrationMask()
const double kMinPictureInkShare = 0.05;  // AUTO observation: ink share inside the mapped picture rectangle
const double kMinUnionShare = 0.9;        // AUTO observation: output ink box against the box of text plus picture

struct Scenario {
  const char* name;
  int dpi;
  output::ColorMode colorMode;
};

const Scenario kScenario300Bw = {"mixed_300_bw", 300, output::BLACK_AND_WHITE};
const Scenario kScenario600Bw = {"mixed_600_bw", 600, output::BLACK_AND_WHITE};

std::string num(const double value) {
  return QString::number(value, 'g', 10).toStdString();
}

void reportMetric(const std::string& scenario, const char* metric, const double value, const std::string& bound) {
  BOOST_TEST_MESSAGE("TRUTH-METRIC suite=mixed scenario=" << scenario << " metric=" << metric << " value=" << num(value)
                                                          << " bound=" << bound);
}

test_support::PageSpec mixedSpec(const int dpi) {
  test_support::PageSpec spec;
  spec.kind = test_support::PageKind::MIXED;
  spec.dpi = dpi;
  spec.seed = 1;
  spec.skewDeg = 0.0;
  return spec;
}

// Generated pages are shared between the cases: rendering a 600 dpi page in Debug takes seconds.
const test_support::GeneratedPage& mixedPage(const int dpi) {
  static std::map<int, test_support::GeneratedPage> cache;
  auto it = cache.find(dpi);
  if (it == cache.end()) {
    it = cache.emplace(dpi, test_support::generatePage(mixedSpec(dpi))).first;
  }
  return it->second;
}

// A rectangle of the flat frame at the input dpi as it lies in the output after registration: scaled by the ratio of
// output to input dpi and moved by the registration offset.
QRect mapToOutput(const QRect& flatRect, const int inputDpi, const QPoint& offset) {
  const double scale = static_cast<double>(kOutputDpi) / inputDpi;
  const QPoint topLeft(static_cast<int>(std::lround(flatRect.left() * scale)),
                       static_cast<int>(std::lround(flatRect.top() * scale)));
  const QSize size(static_cast<int>(std::lround(flatRect.width() * scale)),
                   static_cast<int>(std::lround(flatRect.height() * scale)));
  return QRect(topLeft, size).translated(offset);
}

// The text mask of the page at the output dpi, with a thin ink bar added at the right edge of the picture rectangle.
// registerMask() aligns the centre of the output's non-background box with the centre of the mask's ink box before it
// searches +-4 px; on this page the output box covers text and picture, the text mask alone only the text.  The bar
// makes the two boxes equal; it moves the mismatch of the search by a few pixels and nothing else.
QImage registrationMask(const test_support::PageSpec& spec, const test_support::GroundTruth& truth) {
  QImage mask = test_support::renderInkMask(spec, kOutputDpi, 0.0);
  const double scale = static_cast<double>(kOutputDpi) / spec.dpi;
  const int right = static_cast<int>(std::lround((truth.pictureRect.right() + 1) * scale));
  const int top = static_cast<int>(std::lround(truth.pictureRect.top() * scale));
  const int bottom = static_cast<int>(std::lround((truth.pictureRect.bottom() + 1) * scale));
  for (int y = top; y < bottom && y < mask.height(); ++y) {
    uchar* const line = mask.scanLine(y);
    for (int x = right - kPictureBarWidthPx; x < right && x < mask.width(); ++x) {
      line[x] = 0;
    }
  }
  return mask;
}

bool hasInk(const QImage& gray, const QRect& region) {
  const QRect r = region.intersected(gray.rect());
  for (int y = r.top(); y <= r.bottom(); ++y) {
    const uchar* const line = gray.constScanLine(y);
    for (int x = r.left(); x <= r.right(); ++x) {
      if (line[x] < 128) {
        return true;
      }
    }
  }
  return false;
}

// Number of generator specks that left ink in the output.
int specksPresent(const QImage& output, const test_support::GroundTruth& truth, const int dpi, const QPoint& offset) {
  const QImage gray = output.convertToFormat(QImage::Format_Grayscale8);
  int count = 0;
  for (const QRect& speck : truth.specks) {
    const QRect mapped
        = mapToOutput(speck, dpi, offset).adjusted(-kSpeckGrowPx, -kSpeckGrowPx, kSpeckGrowPx, kSpeckGrowPx);
    count += hasInk(gray, mapped) ? 1 : 0;
  }
  return count;
}

// Number of 8-connected ink components (grey < 128) inside region.
int inkComponents(const QImage& output, const QRect& region) {
  const QImage gray = output.convertToFormat(QImage::Format_Grayscale8);
  const QRect r = region.intersected(gray.rect());
  if (r.isEmpty()) {
    return 0;
  }
  const int w = r.width();
  const int h = r.height();
  std::vector<unsigned char> visited(static_cast<size_t>(w) * h, 0);
  std::vector<int> stack;
  int components = 0;
  for (int y0 = 0; y0 < h; ++y0) {
    for (int x0 = 0; x0 < w; ++x0) {
      const size_t index0 = static_cast<size_t>(y0) * w + x0;
      if (visited[index0] || gray.constScanLine(r.top() + y0)[r.left() + x0] >= 128) {
        continue;
      }
      ++components;
      visited[index0] = 1;
      stack.push_back(static_cast<int>(index0));
      while (!stack.empty()) {
        const int index = stack.back();
        stack.pop_back();
        const int x = index % w;
        const int y = index / w;
        for (int dy = -1; dy <= 1; ++dy) {
          for (int dx = -1; dx <= 1; ++dx) {
            const int nx = x + dx;
            const int ny = y + dy;
            if (nx < 0 || ny < 0 || nx >= w || ny >= h) {
              continue;
            }
            const size_t nIndex = static_cast<size_t>(ny) * w + nx;
            if (!visited[nIndex] && gray.constScanLine(r.top() + ny)[r.left() + nx] < 128) {
              visited[nIndex] = 1;
              stack.push_back(static_cast<int>(nIndex));
            }
          }
        }
      }
    }
  }
  return components;
}

PipelineResult runMixed(const int dpi,
                        const output::ColorMode colorMode,
                        const double despeckleLevel,
                        const page_split::LayoutType layout) {
  PipelineOptions options;
  options.layout = layout;
  options.colorMode = colorMode;
  options.despeckleLevel = despeckleLevel;
  options.outputDpi = kOutputDpi;
  return runPipeline(mixedPage(dpi).image, options);
}

// Everything measured for one black-and-white scenario.
struct BwMeasured {
  PipelineResult withDespeckle;
  PipelineResult withoutDespeckle;
  QPoint offset;
  int specksTotal = 0;
  int specksReachable = 0;  // inside the box of text and picture: the only part of the page the output keeps
  int specksReachablePresentOff = 0;
  int specksPresentOff = 0;
  int specksPresentDefault = 0;
  double removedShare = 0.0;
  int glyphComponentsOff = 0;
  int glyphComponentsDefault = 0;
  double glyphChange = 0.0;
};

BwMeasured measureBw(const Scenario& scenario) {
  BwMeasured m;
  const test_support::GeneratedPage& page = mixedPage(scenario.dpi);
  m.withDespeckle = runMixed(scenario.dpi, scenario.colorMode, 1.0, page_split::SINGLE_PAGE_UNCUT);
  m.withoutDespeckle = runMixed(scenario.dpi, scenario.colorMode, 0.0, page_split::SINGLE_PAGE_UNCUT);
  BOOST_REQUIRE_EQUAL(m.withDespeckle.pages.size(), 1u);
  BOOST_REQUIRE_EQUAL(m.withoutDespeckle.pages.size(), 1u);
  const QImage& outDefault = m.withDespeckle.pages.front().output;
  const QImage& outOff = m.withoutDespeckle.pages.front().output;
  BOOST_REQUIRE_MESSAGE(!outDefault.isNull() && !outOff.isNull(), scenario.name << ": no output image");
  BOOST_REQUIRE_MESSAGE(outDefault.size() == outOff.size(),
                        scenario.name << ": despeckling changed the output size, the two runs cannot share one offset");

  // Despeckling does not move content, so the offset found on the cleaner default output serves both runs.
  const QImage mask = registrationMask(mixedSpec(scenario.dpi), page.truth);
  m.offset = test_support::registerMask(outDefault, mask, kRegistrationRadius).offset;

  m.specksTotal = static_cast<int>(page.truth.specks.size());
  m.specksPresentOff = specksPresent(outOff, page.truth, scenario.dpi, m.offset);
  // The output stage keeps the content box (text and picture) and fills the rest of the page with paper, so a speck
  // outside that box is not in the output however the despeckling is set.  Only specks inside it can test despeckling.
  {
    const QRect contentBox = page.truth.textBlock.united(page.truth.pictureRect);
    test_support::GroundTruth reachable = page.truth;
    reachable.specks.clear();
    for (const QRect& speck : page.truth.specks) {
      if (contentBox.contains(speck)) {
        reachable.specks.push_back(speck);
      }
    }
    m.specksReachable = static_cast<int>(reachable.specks.size());
    m.specksReachablePresentOff = specksPresent(outOff, reachable, scenario.dpi, m.offset);
  }
  m.specksPresentDefault = specksPresent(outDefault, page.truth, scenario.dpi, m.offset);
  m.removedShare
      = m.specksPresentOff > 0 ? 1.0 - static_cast<double>(m.specksPresentDefault) / m.specksPresentOff : 0.0;

  const QRect textRegion = mapToOutput(page.truth.textBlock, scenario.dpi, m.offset)
                               .adjusted(-kTextBlockGrowPx, -kTextBlockGrowPx, kTextBlockGrowPx, kTextBlockGrowPx);
  m.glyphComponentsOff = inkComponents(outOff, textRegion);
  m.glyphComponentsDefault = inkComponents(outDefault, textRegion);
  m.glyphChange
      = m.glyphComponentsOff > 0
            ? std::fabs(static_cast<double>(m.glyphComponentsDefault - m.glyphComponentsOff)) / m.glyphComponentsOff
            : 1.0;
  return m;
}

void reportBw(const Scenario& scenario, const BwMeasured& m) {
  const std::string name = scenario.name;
  reportMetric(name, "layout_type", m.withDespeckle.layoutType, "none");
  reportMetric(name, "registration_offset_x_px", m.offset.x(), "none");
  reportMetric(name, "registration_offset_y_px", m.offset.y(), "none");
  reportMetric(name, "specks_total", m.specksTotal, "none");
  reportMetric(name, "specks_present_despeckle_off", m.specksPresentOff, "none");
  reportMetric(name, "specks_present_despeckle_off_share_of_all",
               m.specksTotal > 0 ? static_cast<double>(m.specksPresentOff) / m.specksTotal : 0.0, "none");
  reportMetric(name, "specks_inside_content_box", m.specksReachable, num(kMinReachableSpecks));
  reportMetric(name, "specks_inside_content_box_present_despeckle_off", m.specksReachablePresentOff, "none");
  reportMetric(name, "specks_inside_content_box_present_despeckle_off_share",
               m.specksReachable > 0 ? static_cast<double>(m.specksReachablePresentOff) / m.specksReachable : 0.0,
               num(kMinSpecksPresentOff));
  reportMetric(name, "specks_present_despeckle_default", m.specksPresentDefault, "none");
  reportMetric(name, "specks_removed_share", m.removedShare, num(kMinSpeckRemovedShare));
  reportMetric(name, "glyph_components_despeckle_off", m.glyphComponentsOff, "none");
  reportMetric(name, "glyph_components_despeckle_default", m.glyphComponentsDefault, "none");
  reportMetric(name, "glyph_component_change", m.glyphChange, num(kMaxGlyphComponentChange));
  const SubPageResult& sub = m.withDespeckle.pages.front();
  reportMetric(name, "output_w_px", sub.outputSize.width(), "none");
  reportMetric(name, "output_h_px", sub.outputSize.height(), "none");
  reportMetric(name, "content_w_px", sub.contentRect.width(), "none");
  reportMetric(name, "content_h_px", sub.contentRect.height(), "none");
}

void checkBw(const Scenario& scenario, const BwMeasured& m) {
  const std::string name = scenario.name;
  BOOST_CHECK_EQUAL(m.withDespeckle.layoutType, 0);
  BOOST_REQUIRE_MESSAGE(m.specksTotal > 0, name << ": the generator placed no specks");
  BOOST_REQUIRE_MESSAGE(m.specksReachable >= kMinReachableSpecks,
                        name << ": only " << m.specksReachable << " specks lie inside the content box, too few to test "
                             << "despeckling");
  const double presentShare = static_cast<double>(m.specksReachablePresentOff) / m.specksReachable;
  BOOST_REQUIRE_MESSAGE(presentShare >= kMinSpecksPresentOff,
                        name << ": only " << num(presentShare) << " of the specks inside the content box are present "
                             << "with despeckling off, the fixture cannot test despeckling");
  BOOST_CHECK_MESSAGE(m.removedShare >= kMinSpeckRemovedShare, name << ": default despeckling removed "
                                                                    << num(m.removedShare) << " of the specks, below "
                                                                    << num(kMinSpeckRemovedShare));
  BOOST_CHECK_MESSAGE(m.glyphChange <= kMaxGlyphComponentChange, name << ": glyph components changed by "
                                                                      << num(m.glyphChange) << ", above "
                                                                      << num(kMaxGlyphComponentChange));
  const SubPageResult& sub = m.withDespeckle.pages.front();
  BOOST_CHECK_EQUAL(sub.outputDpiX, kOutputDpi);
  BOOST_CHECK_EQUAL(sub.outputDpiY, kOutputDpi);
}

void runBwCase(const Scenario& scenario) {
  const BwMeasured m = measureBw(scenario);
  reportBw(scenario, m);
  checkBw(scenario, m);
}

}  // namespace

BOOST_AUTO_TEST_SUITE(MixedPageTestSuite)

BOOST_AUTO_TEST_CASE(test_mixed_300_bw_despeckle) {
  runBwCase(kScenario300Bw);
}

BOOST_AUTO_TEST_CASE(test_mixed_600_bw_despeckle) {
  runBwCase(kScenario600Bw);
}

// Research P3: page_split AUTO cuts the mixed page and drops its picture.  The coverage scenarios force
// SINGLE_PAGE_UNCUT; this case observes the AUTO behaviour and keeps it as a marked defect (D-18).
BOOST_AUTO_TEST_CASE(test_mixed_auto_layout_keeps_single_page) {
  const int dpi = 300;
  const test_support::GeneratedPage& page = mixedPage(dpi);
  const PipelineResult result = runMixed(dpi, output::BLACK_AND_WHITE, 1.0, page_split::AUTO_LAYOUT_TYPE);
  BOOST_REQUIRE_EQUAL(result.pages.size(), 1u);
  const QImage& out = result.pages.front().output;
  BOOST_REQUIRE_MESSAGE(!out.isNull(), "AUTO layout: no output image");

  const std::string name = "mixed_300_auto_layout";
  reportMetric(name, "layout_type", result.layoutType, "0");
  reportMetric(name, "cutters", static_cast<double>(result.cutters.size()), "none");
  for (size_t i = 0; i < result.cutters.size(); ++i) {
    const QLineF& line = result.cutters[i];
    reportMetric(name, ("cutter_" + std::to_string(i) + "_x1_px").c_str(), line.x1(), "none");
    reportMetric(name, ("cutter_" + std::to_string(i) + "_y1_px").c_str(), line.y1(), "none");
    reportMetric(name, ("cutter_" + std::to_string(i) + "_x2_px").c_str(), line.x2(), "none");
    reportMetric(name, ("cutter_" + std::to_string(i) + "_y2_px").c_str(), line.y2(), "none");
  }
  reportMetric(name, "output_w_px", out.width(), "none");
  reportMetric(name, "output_h_px", out.height(), "none");

  // The page is kept whole when the output's ink box spans text and picture.  The ink box does not depend on the
  // registration, which cannot be trusted when the page was cut; the picture ink share does, and counts only when the
  // layout is uncut.
  const QRect box = test_support::nonBackgroundBox(out);
  const double scale = static_cast<double>(kOutputDpi) / dpi;
  const QRect truthUnion = page.truth.textBlock.united(page.truth.pictureRect);
  const double widthShare = box.isNull() ? 0.0 : box.width() / (truthUnion.width() * scale);
  const double heightShare = box.isNull() ? 0.0 : box.height() / (truthUnion.height() * scale);
  reportMetric(name, "ink_box_w_px", box.width(), "none");
  reportMetric(name, "ink_box_h_px", box.height(), "none");
  reportMetric(name, "text_block_w_px", page.truth.textBlock.width() * scale, "none");
  reportMetric(name, "ink_box_width_share_of_text_and_picture", widthShare, num(kMinUnionShare));
  reportMetric(name, "ink_box_height_share_of_text_and_picture", heightShare, num(kMinUnionShare));

  const QImage mask = registrationMask(mixedSpec(dpi), page.truth);
  const QPoint offset = test_support::registerMask(out, mask, kRegistrationRadius).offset;
  const QRect pictureInOutput = mapToOutput(page.truth.pictureRect, dpi, offset)
                                    .adjusted(kPictureShrinkPx, kPictureShrinkPx, -kPictureShrinkPx, -kPictureShrinkPx);
  double inkShare = 0.0;
  if (out.rect().contains(pictureInOutput)) {
    const QImage gray = out.convertToFormat(QImage::Format_Grayscale8);
    long long ink = 0;
    for (int y = pictureInOutput.top(); y <= pictureInOutput.bottom(); ++y) {
      const uchar* const line = gray.constScanLine(y);
      for (int x = pictureInOutput.left(); x <= pictureInOutput.right(); ++x) {
        ink += line[x] < 128 ? 1 : 0;
      }
    }
    inkShare = static_cast<double>(ink) / (static_cast<double>(pictureInOutput.width()) * pictureInOutput.height());
  }
  reportMetric(name, "picture_ink_share_at_registered_offset", inkShare, num(kMinPictureInkShare));

  const bool keepsSinglePage = result.layoutType == 0;
  const bool keepsPage
      = widthShare >= kMinUnionShare && heightShare >= kMinUnionShare && inkShare >= kMinPictureInkShare;
  XFAIL_CHECK(keepsSinglePage && keepsPage,
              "page_split AUTO cuts the mixed page and keeps only a fragment of its picture, dropping the text: no "
              "requirement");
}

BOOST_AUTO_TEST_SUITE_END()
