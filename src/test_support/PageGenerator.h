// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_PAGEGENERATOR_H_
#define SCANTAILOR_TEST_SUPPORT_PAGEGENERATOR_H_

#include <QByteArray>
#include <QImage>
#include <QLineF>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <cstdint>
#include <vector>

namespace test_support {

// Fixture designs.  Geometry is given in base units of 1/300 in and scaled by dpi / 300 (rounded to whole
// pixels, exact at 300 and 600 dpi).  All text is drawn from a 5x7 dot font: one glyph dot is 5 base px, a
// glyph cell is 6 dots wide (one dot of letter spacing), a text line pitch is 11 dots.
//
//   FLAT    1000 x 1150.  13 lines of 20 glyph cells with word gaps, block centred on the page.
//   SPREAD  1654 x 1181.  Two text blocks of 21 cells x 15 lines (frames 640 x 860) with their top-left
//           corners at (73, 160) and (940, 160): a 227 px gutter centred at x = 826.5.  A shadow band of
//           120 px centred on the gutter darkens the paper by up to 25 levels (quadratic profile).
//   MIXED   1000 x 1150.  One text column of 14 cells x 15 lines at (100, 150) (frame 420 x 850), a picture
//           rectangle x 580..900, y 300..700 filled with colour value noise (16 px grid, channel values
//           40..220), and 40 ink-coloured square specks of 1 to 3 px side in the paper area, at least
//           20 px from text, picture and page edge.
//   CURVED  1000 x 1150.  A space-free block of 23 cells x 14 lines (frame 700 x 800) centred on the page,
//           warped as a page curling at its right (spine) edge.  With p(u) = 1 - sqrt(1 - u^2) and u the
//           position across the block from the far (left) edge, 0, to the spine edge, 1: the image of the
//           top text line is displaced by -A p, of the bottom line by +A p, of lines in between by linear
//           interpolation in y, and x is compressed towards the spine by 0.15 A p.  A = 25 base px.  The
//           profile is evaluated at the image x coordinate so that the image-to-flat map has a closed form.
//
// In every kind the text block, picture and speck positions in GroundTruth are in the flat (unrotated,
// unwarped) frame; skewDeg rotates the whole page about its centre afterwards.
enum class PageKind { FLAT, SPREAD, MIXED, CURVED };

struct PageSpec {
  PageKind kind = PageKind::FLAT;
  /// 75 to 1200.  Anything else gives a null image.  300 and 600 are the dpi values the suites use.
  int dpi = 300;
  uint64_t seed = 1;
  /// Rotation applied about the page centre, in degrees, |skewDeg| <= 5.7 (the hand-written series
  /// below is only valid for small angles).  A positive value turns the page content clockwise as
  /// seen on screen (y axis pointing down).  Measured with the unmodified deskew stage: the angle it
  /// stores to straighten such a page is the negative of this value (skewDeg 1.5 gives -1.5).
  double skewDeg = 0.0;
};

struct GroundTruth {
  int dpi = 0;  // 0 for a spec that produced no page
  double skewDeg = 0.0;
  QSize pageSize;             // input pixels
  QRect textBlock;            // tight box around all text ink, flat frame, input pixels (SPREAD: both blocks)
  int lineCount = 0;          // lines per text block
  std::vector<int> lineTops;  // flat frame, input pixels, one per text line (SPREAD: the same for both blocks)

  // SPREAD
  QRect leftBlock, rightBlock;  // tight ink boxes of the two blocks, flat frame
  QLineF gutterCentre;          // from the page top to the page bottom, input coordinates after rotation
  double gutterHalfWidthPx = 0.0;

  // MIXED
  QRect pictureRect;          // flat frame
  std::vector<QRect> specks;  // flat frame

  // CURVED
  double curlAmplitudePx = 0.0;
};

struct GeneratedPage {
  QImage image;  // Format_RGB32, dotsPerMeterX/Y = int(dpi / 0.0254 + 0.5); null for an unsupported dpi
  GroundTruth truth;
};

/**
 * Renders a page of seeded pseudo text.  Uses no font, no painter and no library random or
 * trigonometric function, so the pixels are identical on every toolchain.
 * Returns a null image and a GroundTruth with dpi 0 for a dpi outside 75..1200.
 * Throws std::invalid_argument for an angle outside the supported range.
 */
GeneratedPage generatePage(const PageSpec& spec);

/**
 * The text glyph ink of the page described by spec (no picture, no specks, no shadow), rendered at
 * targetDpi into Format_Grayscale8 with 0 = ink and 255 = paper.  The mask is in the flat frame of the
 * page (spec.skewDeg is ignored, a curved page is not warped) and then rotated by extraRotationDeg about
 * the centre of the text block.  A target pixel is ink when at least 5 of its 3x3 samples are.
 * Returns a null image for a dpi outside 75..1200 and throws std::invalid_argument for an angle outside
 * the supported range.
 */
QImage renderInkMask(const PageSpec& spec, int targetDpi, double extraRotationDeg);

/// Lowercase hex SHA-256 over the RGB32 scanline bytes of the image (width * 4 bytes per line, padding
/// excluded).  Empty for a null image.
QByteArray fingerprint(const QImage& image);

/// Forward geometric map of the page kind, flat frame to input pixels: the warp of a CURVED page (solved
/// by fixed-point iteration, 20 steps at most), then the rotation by spec.skewDeg.  Both maps throw
/// std::invalid_argument for a dpi outside 75..1200 or an angle outside the supported range.
QPointF mapFlatToImage(const PageSpec& spec, const QPointF& flatPoint);

/// Inverse of mapFlatToImage, the map the renderer samples with.  mapFlatToImage inverts it to 1e-6 px
/// except within 0.01 px of the spine edge of a CURVED block, where the profile has a vertical tangent.
QPointF mapImageToFlat(const PageSpec& spec, const QPointF& imagePoint);

}  // namespace test_support

#endif  // SCANTAILOR_TEST_SUPPORT_PAGEGENERATOR_H_
