// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_TRUTHMETRICS_H_
#define SCANTAILOR_TEST_SUPPORT_TRUTHMETRICS_H_

#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QString>

namespace test_support {

// Metrics that measure pipeline output against the ground truth of PageGenerator.  They are plain Qt
// and arithmetic (no imageproc), so a defect in the code under test cannot hide in the measurement.
// Grey is qGray (11 R + 16 G + 5 B) / 32; ink is grey < 128.  Images of any format are accepted.

struct SanityCheck {
  bool ok;
  QString reason;  // empty when ok, otherwise names the first failed condition
};

struct SanityExpectation {
  QSize minSize;  // inclusive
  QSize maxSize;  // inclusive; a non-positive width or height means no upper limit in that direction
  double minInk;  // allowed range of inkFraction
  double maxInk;
  bool portrait;  // the output must not be wider than it is high
};

/// Share of pixels with grey < 128; 0 for a null image.
double inkFraction(const QImage& image);

/// Bounding box of the pixels that are not background: grey < 200 or chroma (max - min of R, G, B) > 30.
/// Null when there is none.
QRect nonBackgroundBox(const QImage& image);

/// Number of text lines in region: runs of consecutive rows whose ink count is at least 2% of the region
/// width (and at least one pixel).  The region is clipped to the image.
int countHorizontalBands(const QImage& image, const QRect& region);

/// Intersection over union; 0 when the union is empty.
double boxIoU(const QRectF& a, const QRectF& b);

/// boxIoU of the two boxes after each is expressed relative to the centre of its own frame, so that a
/// content box found in a deskewed or cropped frame can be compared with the truth in the input frame.
double centredBoxIoU(const QRectF& detected,
                     const QPointF& detectedFrameCentre,
                     const QRectF& truth,
                     const QPointF& truthFrameCentre);

struct Registration {
  /// Translation that moves a truth mask pixel (x, y) onto the output pixel (x + offset.x, y + offset.y).
  QPoint offset;
  long long mismatch;  // pixels whose ink state differs at that offset
};

/**
 * Aligns truthMask (Format_Grayscale8, 0 = ink) with the binarised output: the centres of
 * nonBackgroundBox(output) and of the mask's ink box are aligned first, then every integer offset within
 * +-searchRadius of that is tried and the one with the fewest differing pixels wins (the smallest
 * displacement from the aligned centres on a tie).  Pixels outside either image count as paper.
 */
Registration registerMask(const QImage& output, const QImage& truthMask, int searchRadius);

/**
 * Share of the truth ink pixels that the output gets wrong at the given offset, tolerant at edges: a
 * pixel where output and truth differ counts only if the truth mask has no pixel with the output's value
 * within edgeRadius (Chebyshev) of it, or the output has no pixel with the truth's value within edgeRadius
 * of it.  Each pixel counts once.  Returns 1 when the truth mask has no ink, so that an empty truth can
 * never pass a tolerance.
 */
double truthMismatchFraction(const QImage& output, const QImage& truthMask, const QPoint& offset, int edgeRadius);

struct StraightnessResult {
  bool valid;  // false when fewer than two strips are usable: the residual is then meaningless
  double maxResidualPx;
  int linesFound;  // lines fitted: expectedLines when valid, otherwise 0
  int stripsUsed;
};

/**
 * Splits textRegion into `strips` equal vertical strips; a strip is usable when its horizontal ink
 * projection has exactly expectedLines bands (see countHorizontalBands).  For every line the ink-weighted
 * centre row of its band in each usable strip is fitted with a least-squares straight line over the strip
 * centres, and the result is the largest absolute residual over all lines and strips.  A tilted straight
 * line has residual 0.
 */
StraightnessResult straightnessResidual(const QImage& image, const QRect& textRegion, int expectedLines, int strips);

/// Checks size range, ink fraction and orientation, in that order; reports the first failure.  A null image fails
/// first.
SanityCheck outputSanity(const QImage& output, const SanityExpectation& expectation);

}  // namespace test_support

#endif  // SCANTAILOR_TEST_SUPPORT_TRUTHMETRICS_H_
