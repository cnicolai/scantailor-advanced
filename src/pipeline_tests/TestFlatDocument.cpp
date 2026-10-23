// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <PageGenerator.h>

#include <QImage>
#include <QRectF>
#include <QString>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <string>

#include "HeadlessPipeline.h"

namespace {

// D-15 start values.
const double kAppliedSkewDeg = 1.5;
const double kAngleToleranceDeg = 0.15;
const double kMinContentIou = 0.95;
const double kMinInkFraction = 0.005;
const double kMaxInkFraction = 0.5;
const int kOutputDpi = 600;
const double kMinOutputWidthFactor = 2.0;  // 600 dpi output of a 300 dpi input

std::string num(const double value) {
  return QString::number(value, 'f', 4).toStdString();
}

double intersectionOverUnion(const QRectF& a, const QRectF& b) {
  const QRectF intersection = a.intersected(b);
  const double intersectionArea = intersection.isValid() ? intersection.width() * intersection.height() : 0.0;
  const double unionArea = a.width() * a.height() + b.width() * b.height() - intersectionArea;
  return unionArea > 0.0 ? intersectionArea / unionArea : 0.0;
}

double blackFraction(const QImage& image) {
  const QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
  long black = 0;
  for (int y = 0; y < gray.height(); ++y) {
    const uchar* const line = gray.constScanLine(y);
    for (int x = 0; x < gray.width(); ++x) {
      if (line[x] < 128) {
        ++black;
      }
    }
  }
  return static_cast<double>(black) / (static_cast<double>(gray.width()) * gray.height());
}

}  // namespace

BOOST_AUTO_TEST_SUITE(FlatDocumentTestSuite)

BOOST_AUTO_TEST_CASE(test_flat_300_bw_deskew_and_content_box) {
  test_support::PageSpec spec;
  spec.kind = test_support::PageKind::FLAT;
  spec.dpi = 300;
  spec.seed = 1;
  spec.skewDeg = kAppliedSkewDeg;
  const test_support::GeneratedPage page = test_support::generatePage(spec);

  const PipelineResult result = runPipeline(page.image, PipelineOptions());
  BOOST_REQUIRE_EQUAL(result.pages.size(), 1u);
  const SubPageResult& sub = result.pages.front();

  // The stored angle is the angle that straightens the page, so it has the opposite sign of the angle the
  // generator applied (measured once, see PageGenerator.h).
  const double angleError = std::fabs(sub.deskewAngleDeg + kAppliedSkewDeg);
  BOOST_TEST_MESSAGE("stored deskew angle " << num(sub.deskewAngleDeg) << ", applied " << num(kAppliedSkewDeg));
  BOOST_CHECK_MESSAGE(angleError <= kAngleToleranceDeg,
                      "deskew angle error " << num(angleError) << " exceeds " << num(kAngleToleranceDeg));

  // The content box lives in the deskewed frame, so both boxes are taken relative to their frame centres.
  BOOST_REQUIRE_MESSAGE(sub.pageRect.isValid() && sub.contentRect.isValid(), "select_content found no boxes");
  const QRectF truthBox = QRectF(page.truth.textBlock)
                              .translated(-page.truth.pageSize.width() / 2.0, -page.truth.pageSize.height() / 2.0);
  const QRectF foundBox = QRectF(sub.contentRect).translated(-sub.pageRect.center());
  const double iou = intersectionOverUnion(truthBox, foundBox);
  BOOST_TEST_MESSAGE("content box IoU " << num(iou));
  BOOST_CHECK_MESSAGE(iou >= kMinContentIou, "content box IoU " << num(iou) << " is below " << num(kMinContentIou));

  BOOST_REQUIRE_MESSAGE(!sub.output.isNull(), "no output image");
  const double ink = blackFraction(sub.output);
  BOOST_TEST_MESSAGE("black fraction " << num(ink));
  BOOST_CHECK_MESSAGE(ink >= kMinInkFraction && ink <= kMaxInkFraction,
                      "output black fraction " << num(ink) << " is outside [" << num(kMinInkFraction) << ", "
                                               << num(kMaxInkFraction) << "]");

  BOOST_CHECK_EQUAL(sub.outputDpiX, kOutputDpi);
  BOOST_CHECK_EQUAL(sub.outputDpiY, kOutputDpi);
  BOOST_CHECK_MESSAGE(sub.outputSize.width() >= kMinOutputWidthFactor * page.truth.textBlock.width(),
                      "output width " << sub.outputSize.width() << " is smaller than twice the text block width "
                                      << page.truth.textBlock.width());
}

BOOST_AUTO_TEST_SUITE_END()
