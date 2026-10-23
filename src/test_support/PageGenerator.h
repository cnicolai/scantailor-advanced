// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_PAGEGENERATOR_H_
#define SCANTAILOR_TEST_SUPPORT_PAGEGENERATOR_H_

#include <QImage>
#include <QRect>
#include <QSize>
#include <cstdint>
#include <vector>

namespace test_support {

enum class PageKind { FLAT };  // plan 02-03 adds SPREAD, MIXED, CURVED

struct PageSpec {
  PageKind kind = PageKind::FLAT;
  /// 300 or 600.  Geometry is defined in units of 1/300 in and scaled by dpi / 300.
  int dpi = 300;
  uint64_t seed = 1;
  /// Rotation applied about the page centre, in degrees, |skewDeg| <= 5.7 (the hand-written series
  /// below is only valid for small angles).  A positive value turns the page content clockwise as
  /// seen on screen (y axis pointing down).  Measured with the unmodified deskew stage: the angle it
  /// stores to straighten such a page is the negative of this value (skewDeg 1.5 gives -1.5).
  double skewDeg = 0.0;
};

struct GroundTruth {
  int dpi;
  double skewDeg;
  QSize pageSize;             // input pixels
  QRect textBlock;            // tight box around all ink, unrotated (flat) frame, input pixels
  int lineCount;
  std::vector<int> lineTops;  // flat frame, input pixels, one per text line
};

struct GeneratedPage {
  QImage image;  // Format_RGB32, dotsPerMeterX/Y = int(dpi / 0.0254 + 0.5)
  GroundTruth truth;
};

/**
 * Renders a page of seeded pseudo text.  Uses no font, no painter and no library random or
 * trigonometric function, so the pixels are identical on every toolchain.
 * Throws std::invalid_argument for an unsupported dpi or an angle outside the supported range.
 */
GeneratedPage generatePage(const PageSpec& spec);

}  // namespace test_support

#endif  // SCANTAILOR_TEST_SUPPORT_PAGEGENERATOR_H_
