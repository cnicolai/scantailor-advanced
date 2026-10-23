// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <PageGenerator.h>
#include <TruthMetrics.h>

#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QRgb>
#include <QString>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <functional>
#include <vector>

using namespace test_support;

namespace {

QImage solidGrey(const int width, const int height, const uchar grey) {
  QImage image(width, height, QImage::Format_Grayscale8);
  image.fill(grey);
  return image;
}

QImage whiteWithBlackBox(const int width, const int height, const QRect& box) {
  QImage image = solidGrey(width, height, 255);
  for (int y = box.top(); y <= box.bottom(); ++y) {
    for (int x = box.left(); x <= box.right(); ++x) {
      image.scanLine(y)[x] = 0;
    }
  }
  return image;
}

/// White image with `lines` black bars of `thickness` rows, `pitch` apart, the first starting at `firstTop`.
/// Bar k is displaced in column x by offset(x, k) rows.  Columns where enabled(x) is false stay empty.
QImage barImage(
    const int width,
    const int height,
    const int lines,
    const int pitch,
    const int thickness,
    const int firstTop,
    const std::function<double(int, int)>& offset,
    const std::function<bool(int)>& enabled = [](int) { return true; }) {
  QImage image = solidGrey(width, height, 255);
  for (int k = 0; k < lines; ++k) {
    for (int x = 0; x < width; ++x) {
      if (!enabled(x)) {
        continue;
      }
      const int top = static_cast<int>(std::floor(firstTop + k * pitch + offset(x, k) + 0.5));
      for (int row = top; row < top + thickness; ++row) {
        if (row >= 0 && row < height) {
          image.scanLine(row)[x] = 0;
        }
      }
    }
  }
  return image;
}

QImage flatMask() {
  PageSpec spec;
  spec.kind = PageKind::FLAT;
  spec.dpi = 300;
  spec.seed = 1;
  return renderInkMask(spec, 300, 0.0);
}

long countInk(const QImage& mask) {
  long ink = 0;
  for (int y = 0; y < mask.height(); ++y) {
    for (int x = 0; x < mask.width(); ++x) {
      ink += mask.constScanLine(y)[x] == 0;
    }
  }
  return ink;
}

bool isInk(const QImage& mask, const int x, const int y) {
  return x >= 0 && y >= 0 && x < mask.width() && y < mask.height() && mask.constScanLine(y)[x] == 0;
}

enum class PixelKind { INTERIOR_INK, EDGE_INK, EDGE_PAPER };

bool matches(const QImage& mask, const int x, const int y, const PixelKind kind) {
  int inkNeighbours = 0;
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if ((dx != 0 || dy != 0) && isInk(mask, x + dx, y + dy)) {
        ++inkNeighbours;
      }
    }
  }
  const bool ink = isInk(mask, x, y);
  switch (kind) {
    case PixelKind::INTERIOR_INK:
      return ink && inkNeighbours == 8;
    case PixelKind::EDGE_INK:
      return ink && inkNeighbours >= 3 && inkNeighbours <= 6
             && !(isInk(mask, x - 1, y) && isInk(mask, x + 1, y) && isInk(mask, x, y - 1) && isInk(mask, x, y + 1));
    case PixelKind::EDGE_PAPER:
      return !ink && inkNeighbours >= 3 && inkNeighbours <= 5
             && (isInk(mask, x - 1, y) || isInk(mask, x + 1, y) || isInk(mask, x, y - 1) || isInk(mask, x, y + 1));
  }
  return false;
}

/// Pixels of the given kind in scan order, no two closer than `spacing` in Chebyshev distance.
std::vector<QPoint> pickSpaced(const QImage& mask, const PixelKind kind, const int spacing, const size_t limit) {
  std::vector<QPoint> picked;
  for (int y = 1; y < mask.height() - 1 && picked.size() < limit; ++y) {
    for (int x = 1; x < mask.width() - 1 && picked.size() < limit; ++x) {
      if (!matches(mask, x, y, kind)) {
        continue;
      }
      bool far = true;
      for (const QPoint& other : picked) {
        far = far && (std::abs(other.x() - x) >= spacing || std::abs(other.y() - y) >= spacing);
      }
      if (far) {
        picked.push_back(QPoint(x, y));
      }
    }
  }
  return picked;
}

QImage shifted(const QImage& image, const QPoint& by) {
  QImage out = solidGrey(image.width(), image.height(), 255);
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const int tx = x + by.x();
      const int ty = y + by.y();
      if (tx >= 0 && ty >= 0 && tx < out.width() && ty < out.height()) {
        out.scanLine(ty)[tx] = image.constScanLine(y)[x];
      }
    }
  }
  return out;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(TruthMetricsTestSuite)

BOOST_AUTO_TEST_CASE(test_ink_fraction_extremes) {
  BOOST_CHECK_EQUAL(inkFraction(solidGrey(100, 100, 255)), 0.0);
  BOOST_CHECK_EQUAL(inkFraction(solidGrey(100, 100, 0)), 1.0);
  BOOST_CHECK_EQUAL(inkFraction(solidGrey(100, 100, 127)), 1.0);  // grey < 128 is ink
  BOOST_CHECK_EQUAL(inkFraction(solidGrey(100, 100, 128)), 0.0);
  BOOST_CHECK_CLOSE(inkFraction(whiteWithBlackBox(100, 100, QRect(0, 0, 100, 50))), 0.5, 1e-9);
  BOOST_CHECK_EQUAL(inkFraction(QImage()), 0.0);

  // Colour images are measured through their grey value.
  QImage red(10, 10, QImage::Format_RGB32);
  red.fill(qRgb(255, 0, 0));  // grey 87
  BOOST_CHECK_EQUAL(inkFraction(red), 1.0);
}

BOOST_AUTO_TEST_CASE(test_non_background_box) {
  const QRect square = nonBackgroundBox(whiteWithBlackBox(200, 200, QRect(30, 40, 10, 10)));
  BOOST_CHECK_MESSAGE(square == QRect(30, 40, 10, 10), "box of the black square is ("
                                                           << square.x() << ", " << square.y() << ", " << square.width()
                                                           << " x " << square.height() << ")");
  BOOST_CHECK_MESSAGE(nonBackgroundBox(solidGrey(50, 50, 255)).isNull(), "an all-white image has no content box");
  BOOST_CHECK_MESSAGE(nonBackgroundBox(QImage()).isNull(), "a null image has no content box");

  // A light but saturated pixel (grey 230, chroma 75) is not background; light grey 200 is.
  QImage image(60, 60, QImage::Format_RGB32);
  image.fill(qRgb(255, 255, 255));
  image.setPixel(10, 20, qRgb(255, 230, 180));
  image.setPixel(15, 25, qRgb(199, 199, 199));
  image.setPixel(50, 55, qRgb(200, 200, 200));
  const QRect box = nonBackgroundBox(image);
  BOOST_CHECK_MESSAGE(box == QRect(10, 20, 6, 6), "box of the faint pixels is (" << box.x() << ", " << box.y() << ", "
                                                                                 << box.width() << " x " << box.height()
                                                                                 << ")");
}

BOOST_AUTO_TEST_CASE(test_box_iou) {
  BOOST_CHECK_CLOSE(boxIoU(QRectF(10, 20, 100, 50), QRectF(10, 20, 100, 50)), 1.0, 1e-9);
  BOOST_CHECK_EQUAL(boxIoU(QRectF(0, 0, 10, 10), QRectF(20, 20, 10, 10)), 0.0);
  BOOST_CHECK_CLOSE(boxIoU(QRectF(0, 0, 10, 10), QRectF(5, 0, 10, 10)), 1.0 / 3.0, 1e-9);
  BOOST_CHECK_EQUAL(boxIoU(QRectF(), QRectF()), 0.0);
}

BOOST_AUTO_TEST_CASE(test_centred_box_iou) {
  // The truth block is 600 x 750 around the centre of a 1000 x 1150 frame; the detected box is 604 x 756
  // around the centre of a 640 x 800 frame in another position.
  const QRectF truth(200.0, 200.0, 600.0, 750.0);
  const QPointF truthCentre(500.0, 575.0);
  const QRectF detected(18.0, 22.0, 604.0, 756.0);
  const QPointF detectedCentre(320.0, 400.0);
  const double iou = centredBoxIoU(detected, detectedCentre, truth, truthCentre);
  BOOST_CHECK_MESSAGE(std::fabs(iou - 0.985) < 0.001, "centred IoU is " << iou << ", expected about 0.985");
  BOOST_CHECK_CLOSE(iou, 600.0 * 750.0 / (604.0 * 756.0), 1e-9);

  // Moving both frames and boxes together changes nothing.
  BOOST_CHECK_CLOSE(centredBoxIoU(detected.translated(50, -30), detectedCentre + QPointF(50, -30), truth, truthCentre),
                    iou, 1e-9);
}

BOOST_AUTO_TEST_CASE(test_register_mask_recovers_offset) {
  const QImage mask = flatMask();
  BOOST_REQUIRE(!mask.isNull());
  const QPoint by(3, -2);
  const QImage output = shifted(mask, by);

  const Registration found = registerMask(output, mask, 4);
  BOOST_CHECK_MESSAGE(found.offset == by, "recovered offset (" << found.offset.x() << ", " << found.offset.y() << ")");
  BOOST_CHECK_EQUAL(found.mismatch, 0);

  // The search starts from the aligned centres, so a different frame size does not matter.
  QImage padded = solidGrey(mask.width() + 40, mask.height() + 60, 255);
  for (int y = 0; y < output.height(); ++y) {
    for (int x = 0; x < output.width(); ++x) {
      padded.scanLine(y + 25)[x + 10] = output.constScanLine(y)[x];
    }
  }
  const Registration inPadded = registerMask(padded, mask, 4);
  BOOST_CHECK_MESSAGE(inPadded.offset == by + QPoint(10, 25), "recovered offset (" << inPadded.offset.x() << ", "
                                                                                   << inPadded.offset.y()
                                                                                   << ") in a padded frame");
  BOOST_CHECK_EQUAL(inPadded.mismatch, 0);
}

BOOST_AUTO_TEST_CASE(test_truth_mismatch_edge_tolerance) {
  const QImage truth = flatMask();
  BOOST_REQUIRE(!truth.isNull());
  const long ink = countInk(truth);
  BOOST_REQUIRE(ink > 0);

  BOOST_CHECK_EQUAL(truthMismatchFraction(truth, truth, QPoint(0, 0), 1), 0.0);

  // Flips on ink edges, at least 3 px apart, stay within the tolerance of 1 px.
  QImage edgeFlipped = truth;
  const std::vector<QPoint> edgeInk = pickSpaced(truth, PixelKind::EDGE_INK, 3, 150);
  const std::vector<QPoint> edgePaper = pickSpaced(truth, PixelKind::EDGE_PAPER, 3, 150);
  BOOST_REQUIRE(edgeInk.size() >= 50 && edgePaper.size() >= 50);
  for (const QPoint& p : edgeInk) {
    edgeFlipped.scanLine(p.y())[p.x()] = 255;
  }
  for (const QPoint& p : edgePaper) {
    // Keep the two groups apart as well so that a window never holds two flips.
    bool clear = true;
    for (const QPoint& q : edgeInk) {
      clear = clear && (std::abs(q.x() - p.x()) >= 3 || std::abs(q.y() - p.y()) >= 3);
    }
    if (clear) {
      edgeFlipped.scanLine(p.y())[p.x()] = 0;
    }
  }
  BOOST_CHECK(edgeFlipped != truth);
  BOOST_CHECK_EQUAL(truthMismatchFraction(edgeFlipped, truth, QPoint(0, 0), 1), 0.0);

  // Interior pixels flipped to paper count one each.
  QImage interiorFlipped = truth;
  const std::vector<QPoint> interior = pickSpaced(truth, PixelKind::INTERIOR_INK, 3, 40);
  BOOST_REQUIRE_EQUAL(interior.size(), 40u);
  for (const QPoint& p : interior) {
    interiorFlipped.scanLine(p.y())[p.x()] = 255;
  }
  const double fraction = truthMismatchFraction(interiorFlipped, truth, QPoint(0, 0), 1);
  BOOST_CHECK_MESSAGE(std::fabs(fraction - 40.0 / ink) < 1e-12,
                      "mismatch fraction " << fraction << ", expected " << 40.0 / ink);

  // The offset is applied to the truth mask.
  const QPoint by(3, -2);
  BOOST_CHECK_EQUAL(truthMismatchFraction(shifted(truth, by), truth, by, 1), 0.0);
  BOOST_CHECK_MESSAGE(truthMismatchFraction(shifted(truth, by), truth, QPoint(0, 0), 1) > 0.1,
                      "a wrong offset must give a large mismatch");

  // An empty truth never passes.
  BOOST_CHECK_EQUAL(truthMismatchFraction(truth, solidGrey(truth.width(), truth.height(), 255), QPoint(0, 0), 1), 1.0);
}

BOOST_AUTO_TEST_CASE(test_straightness_of_straight_lines) {
  const int width = 400;
  const int lines = 5;
  // Level lines and lines with a slight tilt are both straight.
  for (const double tilt : {0.0, 0.02}) {
    const QImage image = barImage(width, 280, lines, 40, 4, 30, [tilt](int x, int) { return tilt * x; });
    const StraightnessResult result = straightnessResidual(image, QRect(0, 0, width, 280), lines, 8);
    BOOST_CHECK_MESSAGE(result.valid, "tilt " << tilt << ": result is not valid");
    BOOST_CHECK_MESSAGE(result.maxResidualPx < 0.5, "tilt " << tilt << ": residual " << result.maxResidualPx);
    BOOST_CHECK_EQUAL(result.linesFound, lines);
    BOOST_CHECK_EQUAL(result.stripsUsed, 8);
  }
}

BOOST_AUTO_TEST_CASE(test_straightness_of_bent_lines) {
  // A parabolic arc with 27 px between its ends and its apex.  Sampled at the centres of 8 strips and fitted
  // with a straight line, the largest residual is 0.4375 * 27 = 11.8 px.
  const int width = 400;
  const int lines = 5;
  const double sagitta = 27.0;
  const QImage image = barImage(width, 320, lines, 40, 4, 30, [sagitta, width](int x, int) {
    const double t = (x + 0.5 - width / 2.0) / (width / 2.0);
    return sagitta * (1.0 - t * t);
  });
  const StraightnessResult result = straightnessResidual(image, QRect(0, 0, width, 320), lines, 8);
  BOOST_CHECK(result.valid);
  BOOST_CHECK_MESSAGE(result.maxResidualPx > 10.0 && result.maxResidualPx < 14.0,
                      "residual of the bent lines is " << result.maxResidualPx << ", expected 10 to 14");
  BOOST_CHECK_EQUAL(result.linesFound, lines);
  BOOST_CHECK_EQUAL(result.stripsUsed, 8);
}

BOOST_AUTO_TEST_CASE(test_straightness_invalid_with_one_strip) {
  const int width = 400;
  const int lines = 5;
  const QImage image
      = barImage(width, 280, lines, 40, 4, 30, [](int, int) { return 0.0; }, [](int x) { return x < 50; });
  // Only the first of eight strips holds lines.
  const StraightnessResult oneStrip = straightnessResidual(image, QRect(0, 0, width, 280), lines, 8);
  BOOST_CHECK_MESSAGE(!oneStrip.valid, "one usable strip must not give a valid residual");
  BOOST_CHECK_EQUAL(oneStrip.stripsUsed, 1);
  BOOST_CHECK_EQUAL(oneStrip.linesFound, 0);
  BOOST_CHECK_MESSAGE(!(oneStrip.maxResidualPx <= 1.0e9), "an invalid result must not carry a passing residual");

  // Six lines expected on a page with five: no strip is usable.
  const QImage level = barImage(width, 280, lines, 40, 4, 30, [](int, int) { return 0.0; });
  const StraightnessResult wrongCount = straightnessResidual(level, QRect(0, 0, width, 280), lines + 1, 8);
  BOOST_CHECK(!wrongCount.valid);
  BOOST_CHECK_EQUAL(wrongCount.stripsUsed, 0);

  // Degenerate arguments are invalid, not a pass.
  BOOST_CHECK(!straightnessResidual(level, QRect(0, 0, width, 280), lines, 1).valid);
  BOOST_CHECK(!straightnessResidual(QImage(), QRect(0, 0, width, 280), lines, 8).valid);
  BOOST_CHECK(!straightnessResidual(level, QRect(), lines, 8).valid);
  BOOST_CHECK_EQUAL(countHorizontalBands(level, QRect(0, 0, width, 280)), lines);
}

BOOST_AUTO_TEST_CASE(test_output_sanity_rejections) {
  SanityExpectation expectation;
  expectation.minSize = QSize(200, 200);
  expectation.maxSize = QSize(1000, 1200);
  expectation.minInk = 0.005;
  expectation.maxInk = 0.5;
  expectation.portrait = true;

  const SanityCheck good = outputSanity(whiteWithBlackBox(400, 500, QRect(50, 50, 200, 250)), expectation);
  BOOST_CHECK_MESSAGE(good.ok, "a plausible page was rejected: " << good.reason.toStdString());
  BOOST_CHECK(good.reason.isEmpty());

  struct Case {
    const char* name;
    QImage image;
    const char* word;  // the reason must name the failed condition
  };
  const std::vector<Case> cases = {
      {"blank", solidGrey(400, 500, 255), "ink"},
      {"all black", solidGrey(400, 500, 0), "ink"},
      {"too small", whiteWithBlackBox(100, 100, QRect(10, 10, 50, 50)), "size"},
      {"too large", whiteWithBlackBox(1100, 1300, QRect(100, 100, 400, 400)), "size"},
      {"landscape", whiteWithBlackBox(500, 400, QRect(50, 50, 200, 200)), "portrait"},
      {"null", QImage(), "null"},
  };
  for (const Case& c : cases) {
    const SanityCheck check = outputSanity(c.image, expectation);
    BOOST_CHECK_MESSAGE(!check.ok, c.name << ": not rejected");
    BOOST_CHECK_MESSAGE(
        check.reason.contains(QString::fromLatin1(c.word), Qt::CaseInsensitive),
        c.name << ": reason \"" << check.reason.toStdString() << "\" does not name \"" << c.word << "\"");
  }

  // Landscape is fine when it is not forbidden.
  expectation.portrait = false;
  BOOST_CHECK(outputSanity(whiteWithBlackBox(500, 400, QRect(50, 50, 200, 200)), expectation).ok);
}

BOOST_AUTO_TEST_SUITE_END()
