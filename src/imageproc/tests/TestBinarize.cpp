// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Binarize.h>
#include <BinaryImage.h>
#include <ExpectedFailure.h>
#include <GrayImage.h>
#include <PageGenerator.h>
#include <RasterOp.h>
#include <TruthMetrics.h>

#include <QImage>
#include <QPoint>
#include <QSize>
#include <boost/test/unit_test.hpp>
#include <cstdint>
#include <functional>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

namespace {

// Ground truth for every binarizer: the share of truth ink pixels the binarized page gets wrong, tolerant at edges
// (truthMismatchFraction with radius 1, offset 0), must stay within the bound.  The start bound is 0.5% of the
// ink area; "measured" is the worst case over the fixtures of the case, in Release, see
// .planning/phases/02-golden-baselines/records/unit-tests.md.
constexpr double kStartBound = 0.005;
constexpr int kEdgeRadius = 1;
constexpr int kRegistrationRadius = 4;

// Parameters of the output stage (BlackWhiteOptions.cpp): window 200, Sauvola 0.34, Wolf 1 / 254 / 0.3.
const QSize kWindow(200, 200);

const test_support::PageSpec kPageSpec = [] {
  test_support::PageSpec spec;
  spec.kind = test_support::PageKind::FLAT;
  spec.dpi = 300;
  spec.seed = 7;
  spec.skewDeg = 0.0;
  return spec;
}();

const QImage& truthMask() {
  static const QImage mask = test_support::renderInkMask(kPageSpec, 300, 0.0);
  return mask;
}

const QImage& evenPage() {
  static const QImage page = test_support::generatePage(kPageSpec).image.convertToFormat(QImage::Format_Grayscale8);
  return page;
}

// The paper falls linearly from 235 to 150 grey levels from the left edge to the right edge.  The generated paper is
// about 233 levels (kPaperRgb in PageGenerator.cpp, grey 233), so every pixel is scaled by target(x) / 233 in integer
// arithmetic; ink and text edges lose contrast along with the paper.
const QImage& unevenPage() {
  static const QImage page = [] {
    QImage img = evenPage().copy();
    const int width = img.width();
    for (int y = 0; y < img.height(); ++y) {
      uint8_t* line = img.scanLine(y);
      for (int x = 0; x < width; ++x) {
        const int target = 235 - (85 * x) / (width - 1);
        line[x] = static_cast<uint8_t>((line[x] * target + 116) / 233);
      }
    }
    return img;
  }();
  return page;
}

// The global thresholds build a BinaryImage straight from the source, which accepts Indexed8 (the format every
// production caller passes: transformToGray() and GrayImage), RGB32, ARGB32, RGB16 and Mono but not
// Format_Grayscale8.  They get the same page as an Indexed8 grey image.
QImage asIndexed8Gray(const QImage& page) {
  return GrayImage(page).toQImage();
}

template <typename Fn>
bool doesNotThrow(const Fn& fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return false;
  }
  return true;
}

double mismatch(const BinaryImage& binarized) {
  return test_support::truthMismatchFraction(binarized.toQImage(), truthMask(), QPoint(0, 0), kEdgeRadius);
}

void checkAgainstTruth(const char* method, const char* fixture, const BinaryImage& binarized, const double bound) {
  BOOST_REQUIRE(!binarized.isNull());
  BOOST_REQUIRE(binarized.size() == truthMask().size());
  const double value = mismatch(binarized);
  BOOST_TEST_MESSAGE("BINARIZE-MEASURE method=" << method << " fixture=" << fixture << " mismatch=" << value
                                                << " bound=" << bound);
  BOOST_CHECK_MESSAGE(value <= bound, method << " on " << fixture << ": mismatch " << value << " exceeds " << bound);
}

void checkRegistration(const char* method, const BinaryImage& binarized) {
  const test_support::Registration reg
      = test_support::registerMask(binarized.toQImage(), truthMask(), kRegistrationRadius);
  BOOST_CHECK_MESSAGE(reg.offset == QPoint(0, 0), method << ": mask registers at (" << reg.offset.x() << ", "
                                                         << reg.offset.y() << "), expected (0, 0)");
}

void checkOnEvenPage(const char* method,
                     const std::function<BinaryImage(const QImage&)>& binarize,
                     const double bound,
                     const bool globalThreshold = false) {
  if (globalThreshold) {
    // F-04-1: the global thresholds throw on Format_Grayscale8 input, the format the plan named for the fixture.
    XFAIL_CHECK(doesNotThrow([&] { binarize(evenPage()); }),
                "global thresholds reject Format_Grayscale8 (std::runtime_error): no requirement");
  }
  const BinaryImage binarized = binarize(globalThreshold ? asIndexed8Gray(evenPage()) : evenPage());
  checkAgainstTruth(method, "even", binarized, bound);
  checkRegistration(method, binarized);
}

// Tightened from the measured worst case: every binarizer measured 0 mismatched ink pixels on both pages (Release,
// recorded in .planning/phases/02-golden-baselines/records/unit-tests.md), so each bound is twice the worst case
// rounded up to the step of 0.05 percentage points, and never below one step: 0.05% of the ink area.
constexpr double kTightBound = 0.0005;

// Bounds per binarizer on the evenly lit page (measured worst case 0 for all nine).
constexpr double kOtsuBound = kTightBound;
constexpr double kMokjiBound = kTightBound;
constexpr double kSauvolaBound = kTightBound;
constexpr double kWolfBound = kTightBound;
constexpr double kFoxBound = kTightBound;
constexpr double kWindowBound = kTightBound;
constexpr double kBradleyBound = kTightBound;
constexpr double kGradBound = kTightBound;
constexpr double kEdgeDivBound = kTightBound;

// Bounds on the unevenly lit page (measured worst case 0 for all seven).
constexpr double kSauvolaUnevenBound = kTightBound;
constexpr double kWolfUnevenBound = kTightBound;
constexpr double kFoxUnevenBound = kTightBound;
constexpr double kWindowUnevenBound = kTightBound;
constexpr double kBradleyUnevenBound = kTightBound;
constexpr double kGradUnevenBound = kTightBound;
constexpr double kEdgeDivUnevenBound = kTightBound;

}  // namespace

BOOST_AUTO_TEST_SUITE(BinarizeTestSuite)

// Guards the zero mismatches below against a vacuous metric: a blank page and a page shifted by 3 px must not pass.
BOOST_AUTO_TEST_CASE(test_ground_truth_metric_rejects_wrong_output) {
  BinaryImage blank(evenPage().size(), WHITE);
  BOOST_CHECK_GT(mismatch(blank), 0.9);

  const BinaryImage good = binarizeSauvola(evenPage(), kWindow, 0.34);
  BOOST_REQUIRE_LE(mismatch(good), kStartBound);
  BinaryImage shifted(good.size(), WHITE);
  rasterOp<RopSrc>(shifted, QRect(3, 3, good.width() - 3, good.height() - 3), good, QPoint(0, 0));
  BOOST_CHECK_GT(mismatch(shifted), kStartBound);
}

BOOST_AUTO_TEST_CASE(test_otsu_on_flat_page) {
  checkOnEvenPage("otsu", [](const QImage& src) { return binarizeOtsu(src); }, kOtsuBound, true);
}

BOOST_AUTO_TEST_CASE(test_mokji_on_flat_page) {
  checkOnEvenPage("mokji", [](const QImage& src) { return binarizeMokji(src); }, kMokjiBound, true);
}

BOOST_AUTO_TEST_CASE(test_sauvola_on_flat_page) {
  checkOnEvenPage("sauvola", [](const QImage& src) { return binarizeSauvola(src, kWindow, 0.34); }, kSauvolaBound);
}

BOOST_AUTO_TEST_CASE(test_wolf_on_flat_page) {
  checkOnEvenPage("wolf", [](const QImage& src) { return binarizeWolf(src, kWindow, 1, 254, 0.3); }, kWolfBound);
}

BOOST_AUTO_TEST_CASE(test_fox_on_flat_page) {
  checkOnEvenPage("fox", [](const QImage& src) { return binarizeFox(src, kWindow, 1, 254, 0.3); }, kFoxBound);
}

BOOST_AUTO_TEST_CASE(test_window_on_flat_page) {
  checkOnEvenPage("window", [](const QImage& src) { return binarizeWindow(src, kWindow); }, kWindowBound);
}

BOOST_AUTO_TEST_CASE(test_bradley_on_flat_page) {
  checkOnEvenPage("bradley", [](const QImage& src) { return binarizeBradley(src, kWindow); }, kBradleyBound);
}

BOOST_AUTO_TEST_CASE(test_grad_on_flat_page) {
  checkOnEvenPage("grad", [](const QImage& src) { return binarizeGrad(src, kWindow); }, kGradBound);
}

BOOST_AUTO_TEST_CASE(test_edgediv_on_flat_page) {
  checkOnEvenPage("edgediv", [](const QImage& src) { return binarizeEdgeDiv(src, kWindow); }, kEdgeDivBound);
}

BOOST_AUTO_TEST_CASE(test_local_binarizers_on_uneven_illumination) {
  const QImage& page = unevenPage();
  checkAgainstTruth("sauvola", "uneven", binarizeSauvola(page, kWindow, 0.34), kSauvolaUnevenBound);
  checkAgainstTruth("wolf", "uneven", binarizeWolf(page, kWindow, 1, 254, 0.3), kWolfUnevenBound);
  checkAgainstTruth("fox", "uneven", binarizeFox(page, kWindow, 1, 254, 0.3), kFoxUnevenBound);
  checkAgainstTruth("window", "uneven", binarizeWindow(page, kWindow), kWindowUnevenBound);
  checkAgainstTruth("bradley", "uneven", binarizeBradley(page, kWindow), kBradleyUnevenBound);
  checkAgainstTruth("grad", "uneven", binarizeGrad(page, kWindow), kGradUnevenBound);
  checkAgainstTruth("edgediv", "uneven", binarizeEdgeDiv(page, kWindow), kEdgeDivUnevenBound);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc
