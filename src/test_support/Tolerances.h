// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_TOLERANCES_H_
#define SCANTAILOR_TEST_SUPPORT_TOLERANCES_H_

// The one place for comparator limits (D-12).  The image-kind limits are frozen (plan 02-09): each is the smallest
// step of its grid strictly above the largest Release/Debug spread measured over all recorded goldens, and never
// above the D-12 ceiling.  The measurement and the rule are in
// .planning/phases/02-golden-baselines/records/tolerance-spread.md.  A test that overrides a limit must say why in a
// comment next to the override (D-12).

namespace test_support {
namespace tolerance {

/// Bi-level: fraction of pixels that differ and find no equal pixel in the other image's 3x3 neighbourhood.
/// Measured max 0 (Release vs Debug), ceiling 0.001, see records/tolerance-spread.md.
constexpr double kBilevelEdgeTolerantXorFraction = 0.0001;
/// Bi-level: relative difference of the 8-connected foreground component counts.
/// Measured max 0 (Release vs Debug), ceiling 0.02, see records/tolerance-spread.md.
constexpr double kBilevelComponentCountRelative = 0.001;
/// Grey/colour: mean absolute error over pixels and channels, in levels.
/// Measured max 0 (Release vs Debug), ceiling 0.5, see records/tolerance-spread.md.
constexpr double kColorMeanAbsError = 0.05;
/// Grey/colour: largest absolute channel difference, in levels.
/// Measured max 0 (Release vs Debug), ceiling 8, see records/tolerance-spread.md.
constexpr double kColorMaxAbsError = 1.0;
/// Shift check: offsets in [-r, r]^2 are tried, in pixels.  The shift metric limit is 0 for every kind.
constexpr int kShiftSearchRadius = 2;

/// Text goldens, keys ending in _deg (D-14).
constexpr double kGeometryAngleDeg = 0.01;
/// Text goldens, keys ending in _px (D-14).
constexpr double kGeometryPx = 1.0;
/// Text goldens, keys ending in _mm (D-14).
constexpr double kGeometryMm = 0.05;
/// Text goldens: slack for the binary representation of decimal text, so that "10.0500" vs "10.0000" passes the
/// 0.05 limit.
constexpr double kTextToleranceSlack = 1e-9;

}  // namespace tolerance
}  // namespace test_support

#endif  // SCANTAILOR_TEST_SUPPORT_TOLERANCES_H_
