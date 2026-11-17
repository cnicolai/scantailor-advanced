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
#include <QRgb>
#include <QString>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "HeadlessPipeline.h"

namespace {

// Ground-truth bounds.  Start values, then tightened from the measured worst case in
// .planning/phases/02-golden-baselines/records/scenario-mixed.md ("Tolerance decisions").
// Share of the specks present with despeckling off that the default removes.  Start 0.90; worst case 1 (shortfall 0),
// so one step of 0.05 percentage points.
const double kMinSpeckRemovedShare = 0.9995;
// Relative change of the glyph component count between the two runs.  Start 1%; worst case 0, so one step.
const double kMaxGlyphComponentChange = 0.0005;
// Fixture sanity: with despeckling off at least this share of the generator's specks inside the content box must show
// in the output, and at least kMinReachableSpecks specks must lie there, else the page cannot test despeckling.
const double kMinSpecksPresentOff = 0.90;
const int kMinReachableSpecks = 10;

// Colour/greyscale output: mean chroma (max - min channel) of the output picture over that of the input picture.
// Start 0.5 (shortfall from the ideal ratio 1: 0.5); worst shortfall 0.00075, so twice that rounded up to the step
// 0.005 gives 0.005.
const double kMinPictureChromaRatio = 0.995;
// Colour/greyscale output: mean grey under the truth paper of the text block minus mean grey under the truth ink.
// Start 100 grey levels; the input page has 201 (shortfall 101); worst shortfall of the output 13.9, so twice that
// rounded up to the step of 1 level gives 28 and the bound 201 - 28.
const double kMinInkPaperContrast = 173.0;
// MIXED output: share of the picture (shrunk) that is not bi-level.  Start 0.95; worst shortfall 0.00033, so twice that
// rounded up to the step 0.0005 gives 0.001.
const double kMinPictureNonBilevelShare = 0.999;
// MIXED output: share of the pixels outside the picture (grown) that are pure black or pure white.  Start 0.999;
// worst shortfall 0, so one step of 0.0005.
const double kMinOutsidePictureBilevelShare = 0.9995;

const int kOutputDpi = 600;
// Sanity check of an output before it may become a golden.
const double kMinInkFractionOfOutput = 0.005;
const double kMaxInkFractionOfOutput = 0.6;  // the binarized picture adds ink
const double kSanityWidthSlack = 1.2;
const double kMaxSanityTextMismatch
    = 0.05;                               // share of the truth text ink the output gets wrong at the registered offset
const double kMinNotBilevelShare = 0.05;  // colour and MIXED output: share of pixels that are not bi-level
const int kMaxLineCountDifference = 1;
const int kRegistrationRadius = 4;
const int kSpeckGrowPx = 1;        // a speck is present when its mapped rectangle grown by this holds ink
const int kTextBlockGrowPx = 4;    // glyph components are counted in the mapped text block grown by this
const int kPictureShrinkPx = 4;    // picture checks look inside the mapped picture rectangle shrunk by this
const int kPictureBarWidthPx = 3;  // see registrationMask()
const int kPictureGrowPx = 8;      // MIXED output: pixels outside the mapped picture grown by this must be bi-level
const int kBlackMax = 5;           // a bi-level pixel has all channels at most this ...
const int kWhiteMin = 250;         // ... or all at least this
const double kMinPictureInkShare = 0.05;  // AUTO observation: ink share inside the mapped picture rectangle
const double kMinUnionShare = 0.9;        // AUTO observation: output ink box against the box of text plus picture

struct Scenario {
  const char* name;
  int dpi;
  output::ColorMode colorMode;
};

const Scenario kScenario300Bw = {"mixed_300_bw", 300, output::BLACK_AND_WHITE};
const Scenario kScenario600Bw = {"mixed_600_bw", 600, output::BLACK_AND_WHITE};
const Scenario kScenario300Color = {"mixed_300_color", 300, output::COLOR_GRAYSCALE};
const Scenario kScenario600Color = {"mixed_600_color", 600, output::COLOR_GRAYSCALE};
const Scenario kScenario300Mixed = {"mixed_300_mixed", 300, output::MIXED};

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

QRect shrunk(const QRect& rect, const int px) {
  return rect.adjusted(px, px, -px, -px);
}

// Mean chroma (max - min of R, G, B) over region, clipped to the image.
double meanChroma(const QImage& image, const QRect& region) {
  const QImage rgb = image.convertToFormat(QImage::Format_RGB32);
  const QRect r = region.intersected(rgb.rect());
  double sum = 0.0;
  long long count = 0;
  for (int y = r.top(); y <= r.bottom(); ++y) {
    const QRgb* const line = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));
    for (int x = r.left(); x <= r.right(); ++x) {
      const int hi = std::max(qRed(line[x]), std::max(qGreen(line[x]), qBlue(line[x])));
      const int lo = std::min(qRed(line[x]), std::min(qGreen(line[x]), qBlue(line[x])));
      sum += hi - lo;
      ++count;
    }
  }
  return count > 0 ? sum / static_cast<double>(count) : 0.0;
}

bool isBilevelPixel(const QRgb pixel) {
  const bool black = qRed(pixel) <= kBlackMax && qGreen(pixel) <= kBlackMax && qBlue(pixel) <= kBlackMax;
  const bool white = qRed(pixel) >= kWhiteMin && qGreen(pixel) >= kWhiteMin && qBlue(pixel) >= kWhiteMin;
  return black || white;
}

// Share of the pixels of region (clipped to the image) that are not bi-level.
double nonBilevelShare(const QImage& image, const QRect& region) {
  const QImage rgb = image.convertToFormat(QImage::Format_RGB32);
  const QRect r = region.intersected(rgb.rect());
  long long other = 0;
  long long count = 0;
  for (int y = r.top(); y <= r.bottom(); ++y) {
    const QRgb* const line = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));
    for (int x = r.left(); x <= r.right(); ++x) {
      other += isBilevelPixel(line[x]) ? 0 : 1;
      ++count;
    }
  }
  return count > 0 ? static_cast<double>(other) / static_cast<double>(count) : 0.0;
}

// Share of the pixels outside hole that are pure black or pure white (all channels 0, or all 255).
double pureBlackWhiteShareOutside(const QImage& image, const QRect& hole) {
  const QImage rgb = image.convertToFormat(QImage::Format_RGB32);
  long long pure = 0;
  long long count = 0;
  for (int y = 0; y < rgb.height(); ++y) {
    const QRgb* const line = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));
    for (int x = 0; x < rgb.width(); ++x) {
      if (hole.contains(x, y)) {
        continue;
      }
      const QRgb p = line[x];
      const bool black = qRed(p) == 0 && qGreen(p) == 0 && qBlue(p) == 0;
      const bool white = qRed(p) == 255 && qGreen(p) == 255 && qBlue(p) == 255;
      pure += (black || white) ? 1 : 0;
      ++count;
    }
  }
  return count > 0 ? static_cast<double>(pure) / static_cast<double>(count) : 0.0;
}

// Mean grey (qGray) of image under the mask pixels inside maskRegion that are ink (mask < 128) or paper; the mask pixel
// (x, y) lies on the image pixel (x + offset.x, y + offset.y).
double meanGreyUnderMask(const QImage& image,
                         const QImage& mask,
                         const QPoint& offset,
                         const QRect& maskRegion,
                         const bool ink) {
  const QImage rgb = image.convertToFormat(QImage::Format_RGB32);
  const QRect r = maskRegion.intersected(mask.rect());
  double sum = 0.0;
  long long count = 0;
  for (int y = r.top(); y <= r.bottom(); ++y) {
    const int iy = y + offset.y();
    if (iy < 0 || iy >= rgb.height()) {
      continue;
    }
    const uchar* const maskLine = mask.constScanLine(y);
    const QRgb* const line = reinterpret_cast<const QRgb*>(rgb.constScanLine(iy));
    for (int x = r.left(); x <= r.right(); ++x) {
      const int ix = x + offset.x();
      if (ix < 0 || ix >= rgb.width() || (maskLine[x] < 128) != ink) {
        continue;
      }
      sum += qGray(line[ix]);
      ++count;
    }
  }
  return count > 0 ? sum / static_cast<double>(count) : 0.0;
}

// Mean grey under the truth paper of the text block minus mean grey under the truth ink.  textMask is in the frame of
// image shifted by offset.
double inkPaperContrast(const QImage& image, const QImage& textMask, const QPoint& offset, const QRect& textBlock) {
  return meanGreyUnderMask(image, textMask, offset, textBlock, false)
         - meanGreyUnderMask(image, textMask, offset, textBlock, true);
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

struct SanityResult {
  bool ok;
  QString reason;
};

// Output sanity check of one scenario, applied before a golden is recorded: size, ink fraction, orientation and line
// count of the text block, content where the truth puts it, and for colour and MIXED output that it is not bi-level.
// The orientation is checked on the ink box inside the mapped text block, because the box of text and picture is almost
// square (1608 x 1620 px).
SanityResult mixedSanity(const Scenario& scenario,
                         const test_support::GeneratedPage& page,
                         const QImage& out,
                         const QPoint& offset) {
  const double scale = static_cast<double>(kOutputDpi) / scenario.dpi;
  test_support::SanityExpectation expectation;
  expectation.minSize = QSize(static_cast<int>(scale * page.truth.textBlock.width()),
                              static_cast<int>(scale * page.truth.textBlock.height()));
  expectation.maxSize = QSize(static_cast<int>(scale * page.truth.pageSize.width() * kSanityWidthSlack), 0);
  expectation.minInk = kMinInkFractionOfOutput;
  expectation.maxInk = kMaxInkFractionOfOutput;
  expectation.portrait = false;
  const test_support::SanityCheck basic = test_support::outputSanity(out, expectation);
  if (!basic.ok) {
    return SanityResult{false, basic.reason};
  }

  const QImage mask = test_support::renderInkMask(mixedSpec(scenario.dpi), kOutputDpi, 0.0);
  const QRect maskRegion = mapToOutput(page.truth.textBlock, scenario.dpi, QPoint(0, 0))
                               .adjusted(-kTextBlockGrowPx, -kTextBlockGrowPx, kTextBlockGrowPx, kTextBlockGrowPx)
                               .intersected(mask.rect());
  const QRect outRegion = maskRegion.translated(offset);
  if (!out.rect().contains(outRegion)) {
    return SanityResult{false, QStringLiteral("the text block lies outside the output at the registered offset")};
  }
  const QImage outCrop = out.copy(outRegion);
  const QImage maskCrop = mask.copy(maskRegion);
  const QRect box = test_support::nonBackgroundBox(outCrop);
  if (box.isNull()) {
    return SanityResult{false, QStringLiteral("no ink where the text block should be")};
  }
  if (box.width() > box.height()) {
    return SanityResult{
        false,
        QStringLiteral("glyph ink box %1x%2 is landscape but portrait is expected").arg(box.width()).arg(box.height())};
  }
  const int bands = test_support::countHorizontalBands(outCrop, box);
  if (std::abs(bands - page.truth.lineCount) > kMaxLineCountDifference) {
    return SanityResult{false,
                        QStringLiteral("found %1 text lines, truth has %2").arg(bands).arg(page.truth.lineCount)};
  }
  const double mismatch
      = test_support::truthMismatchFraction(outCrop, maskCrop, QPoint(0, 0), kOutputDpi / scenario.dpi);
  reportMetric(scenario.name, "sanity_ink_fraction", test_support::inkFraction(out), "none");
  reportMetric(scenario.name, "sanity_text_lines", bands, num(page.truth.lineCount));
  reportMetric(scenario.name, "sanity_text_mismatch", mismatch, num(kMaxSanityTextMismatch));
  if (mismatch > kMaxSanityTextMismatch) {
    return SanityResult{false, QStringLiteral("text mismatch %1 exceeds %2").arg(mismatch).arg(kMaxSanityTextMismatch)};
  }
  if (scenario.colorMode != output::BLACK_AND_WHITE) {
    const double notBilevel = nonBilevelShare(out, out.rect());
    reportMetric(scenario.name, "sanity_not_bilevel_share", notBilevel, num(kMinNotBilevelShare));
    if (notBilevel < kMinNotBilevelShare) {
      return SanityResult{false, QStringLiteral("output is bi-level (%1 of the pixels are not)").arg(notBilevel)};
    }
  }
  return SanityResult{true, QString()};
}

const char* colorModeName(const output::ColorMode mode) {
  switch (mode) {
    case output::BLACK_AND_WHITE:
      return "BLACK_AND_WHITE";
    case output::COLOR_GRAYSCALE:
      return "COLOR_GRAYSCALE";
    case output::MIXED:
      return "MIXED";
  }
  return "UNKNOWN";
}

QString intText(const long long value) {
  return QString::number(value);
}

// Stage decisions of the scenario as text golden values; the suffix of a key decides its tolerance.
test_support::TextValues stageDecisions(const Scenario& scenario,
                                        const PipelineResult& result,
                                        const double despeckle) {
  using test_support::formatNumber;
  const SubPageResult& sub = result.pages.front();
  test_support::TextValues values;
  values["seed"] = intText(1);
  values["input_dpi"] = intText(scenario.dpi);
  values["applied_skew_deg"] = formatNumber(0.0);
  values["color_mode"] = QString::fromLatin1(colorModeName(scenario.colorMode));
  values["despeckle_level"] = formatNumber(despeckle);
  values["deskew_angle_deg"] = formatNumber(sub.deskewAngleDeg);
  values["layout_type"] = intText(result.layoutType);
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

// Goldens are recorded only when the sanity checks of the scenario passed.  The scenario's output is the one
// of the default despeckle level; the despeckle-off run and the AUTO observation get no golden.
void checkGoldens(const Scenario& scenario, const PipelineResult& result, const QPoint& offset) {
  const QImage& out = result.pages.front().output;
  const SanityResult sanityResult = mixedSanity(scenario, mixedPage(scenario.dpi), out, offset);
  BOOST_TEST_MESSAGE("TRUTH-METRIC suite=mixed scenario=" << scenario.name
                                                          << " metric=sanity value=" << (sanityResult.ok ? 1 : 0)
                                                          << " bound=1 reason=" << sanityResult.reason.toStdString());
  const test_support::Sanity sanity{sanityResult.ok, sanityResult.reason};
  const test_support::ImageKind kind = scenario.colorMode == output::BLACK_AND_WHITE ? test_support::ImageKind::BILEVEL
                                                                                     : test_support::ImageKind::COLOR;
  CHECK_IMAGE_GOLDEN("mixed", scenario.name, out, kind, sanity);
  CHECK_TEXT_GOLDEN("mixed", scenario.name, stageDecisions(scenario, result, 1.0), sanity);
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
  checkGoldens(scenario, m.withDespeckle, m.offset);
}

// Everything measured for one colour/greyscale or MIXED scenario.
struct ColorMeasured {
  PipelineResult result;
  QPoint offset;
  QRect pictureInOutput;  // mapped picture rectangle, not shrunk
  double inputPictureChroma = 0.0;
  double outputPictureChroma = 0.0;
  double chromaRatio = 0.0;
  double inputContrast = 0.0;
  double contrast = 0.0;
  double pictureNonBilevel = 0.0;
  double outsideBilevel = 0.0;
};

ColorMeasured measureColor(const Scenario& scenario) {
  ColorMeasured m;
  const test_support::GeneratedPage& page = mixedPage(scenario.dpi);
  m.result = runMixed(scenario.dpi, scenario.colorMode, 1.0, page_split::SINGLE_PAGE_UNCUT);
  BOOST_REQUIRE_EQUAL(m.result.pages.size(), 1u);
  const QImage& out = m.result.pages.front().output;
  BOOST_REQUIRE_MESSAGE(!out.isNull(), scenario.name << ": no output image");

  const QImage regMask = registrationMask(mixedSpec(scenario.dpi), page.truth);
  m.offset = test_support::registerMask(out, regMask, kRegistrationRadius).offset;
  m.pictureInOutput = mapToOutput(page.truth.pictureRect, scenario.dpi, m.offset);

  m.inputPictureChroma = meanChroma(page.image, shrunk(page.truth.pictureRect, kPictureShrinkPx));
  m.outputPictureChroma = meanChroma(out, shrunk(m.pictureInOutput, kPictureShrinkPx));
  m.chromaRatio = m.inputPictureChroma > 0.0 ? m.outputPictureChroma / m.inputPictureChroma : 0.0;

  const QImage outputTextMask = test_support::renderInkMask(mixedSpec(scenario.dpi), kOutputDpi, 0.0);
  const QRect outputTextBlock = mapToOutput(page.truth.textBlock, scenario.dpi, QPoint(0, 0));
  m.contrast = inkPaperContrast(out, outputTextMask, m.offset, outputTextBlock);
  const QImage inputTextMask = test_support::renderInkMask(mixedSpec(scenario.dpi), scenario.dpi, 0.0);
  m.inputContrast = inkPaperContrast(page.image, inputTextMask, QPoint(0, 0), page.truth.textBlock);

  m.pictureNonBilevel = nonBilevelShare(out, shrunk(m.pictureInOutput, kPictureShrinkPx));
  m.outsideBilevel = pureBlackWhiteShareOutside(
      out, m.pictureInOutput.adjusted(-kPictureGrowPx, -kPictureGrowPx, kPictureGrowPx, kPictureGrowPx));
  return m;
}

void reportColor(const Scenario& scenario, const ColorMeasured& m) {
  const std::string name = scenario.name;
  const bool mixed = scenario.colorMode == output::MIXED;
  const SubPageResult& sub = m.result.pages.front();
  reportMetric(name, "layout_type", m.result.layoutType, "none");
  reportMetric(name, "registration_offset_x_px", m.offset.x(), "none");
  reportMetric(name, "registration_offset_y_px", m.offset.y(), "none");
  reportMetric(name, "output_w_px", sub.outputSize.width(), "none");
  reportMetric(name, "output_h_px", sub.outputSize.height(), "none");
  reportMetric(name, "input_picture_chroma", m.inputPictureChroma, "none");
  reportMetric(name, "output_picture_chroma", m.outputPictureChroma, "none");
  reportMetric(name, "input_ink_paper_contrast", m.inputContrast, "none");
  if (!mixed) {
    reportMetric(name, "picture_chroma_ratio", m.chromaRatio, num(kMinPictureChromaRatio));
    reportMetric(name, "ink_paper_contrast", m.contrast, num(kMinInkPaperContrast));
  } else {
    reportMetric(name, "output_ink_paper_contrast", m.contrast, "none");
    reportMetric(name, "picture_chroma_ratio", m.chromaRatio, "none");
    reportMetric(name, "picture_non_bilevel_share", m.pictureNonBilevel, num(kMinPictureNonBilevelShare));
    reportMetric(name, "outside_picture_bilevel_share", m.outsideBilevel, num(kMinOutsidePictureBilevelShare));
  }
}

void checkColor(const Scenario& scenario, const ColorMeasured& m) {
  const std::string name = scenario.name;
  BOOST_CHECK_EQUAL(m.result.layoutType, 0);
  if (scenario.colorMode == output::MIXED) {
    BOOST_CHECK_MESSAGE(m.pictureNonBilevel >= kMinPictureNonBilevelShare,
                        name << ": only " << num(m.pictureNonBilevel) << " of the picture is not bi-level, below "
                             << num(kMinPictureNonBilevelShare));
    BOOST_CHECK_MESSAGE(m.outsideBilevel >= kMinOutsidePictureBilevelShare,
                        name << ": only " << num(m.outsideBilevel) << " of the pixels outside the picture are pure "
                             << "black or white, below " << num(kMinOutsidePictureBilevelShare));
  } else {
    BOOST_CHECK_MESSAGE(m.chromaRatio >= kMinPictureChromaRatio, name << ": picture chroma ratio " << num(m.chromaRatio)
                                                                      << " is below " << num(kMinPictureChromaRatio));
    BOOST_CHECK_MESSAGE(m.contrast >= kMinInkPaperContrast, name << ": ink/paper contrast " << num(m.contrast)
                                                                 << " is below " << num(kMinInkPaperContrast));
  }
  const SubPageResult& sub = m.result.pages.front();
  BOOST_CHECK_EQUAL(sub.outputDpiX, kOutputDpi);
  BOOST_CHECK_EQUAL(sub.outputDpiY, kOutputDpi);
}

void runColorCase(const Scenario& scenario) {
  const ColorMeasured m = measureColor(scenario);
  reportColor(scenario, m);
  checkColor(scenario, m);
  checkGoldens(scenario, m.result, m.offset);
}

}  // namespace

BOOST_AUTO_TEST_SUITE(MixedPageTestSuite)

BOOST_AUTO_TEST_CASE(test_mixed_300_bw_despeckle) {
  runBwCase(kScenario300Bw);
}

BOOST_AUTO_TEST_CASE(test_mixed_600_bw_despeckle) {
  runBwCase(kScenario600Bw);
}

BOOST_AUTO_TEST_CASE(test_mixed_300_color_output) {
  runColorCase(kScenario300Color);
}

BOOST_AUTO_TEST_CASE(test_mixed_600_color_output) {
  runColorCase(kScenario600Color);
}

BOOST_AUTO_TEST_CASE(test_mixed_300_mixed_picture_zone) {
  runColorCase(kScenario300Mixed);
}

// Research P3: page_split AUTO cuts the mixed page and drops its picture.  The coverage scenarios force
// SINGLE_PAGE_UNCUT; this case observes the AUTO behaviour and keeps it as a marked defect.
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

// Fingerprints of the generated mixed inputs, so that a change of the generator shows up as a fixture change and not as
// a pipeline change.
BOOST_AUTO_TEST_CASE(test_mixed_fixture_fingerprints) {
  test_support::TextValues values;
  for (const int dpi : {300, 600}) {
    values[QString("mixed_") + QString::number(dpi)]
        = QString::fromLatin1(test_support::fingerprint(mixedPage(dpi).image));
  }
  BOOST_REQUIRE_EQUAL(values.size(), 2u);
  CHECK_TEXT_GOLDEN("mixed", "fixtures", values, (test_support::Sanity{true, QString()}));
}

BOOST_AUTO_TEST_SUITE_END()
