// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef TST_HIDPI_QT_H
#define TST_HIDPI_QT_H

#include <QObject>

/**
 * Retina tests on two offscreen screens: OneX (device pixel ratio 1) and TwoX (ratio 2), set up in main.cpp.
 */
class TestHiDpiQt : public QObject {
  Q_OBJECT
 private slots:
  void initTestCase();
  void testHqPixmapAtDeviceResolution();
  void testHqPixmapAtOneXUnchanged();
  void testHelpersMatchLegacyAtRatio1();
  void testHelpersAtFractionalRatio();
  void testHqGrabMatchesDeviceReference();
  void testHqRebuildsOnScreenChange();
  void testRepeatedRatioEventKeepsHqPixmap();
  void testSmoothingSwitchPhysicalSize();
  void testThumbnailEffectiveSize();
  void testThumbnailFilesKeyedByRatio();
  void testOldThumbnailLeftInPlace();
  void testStaleLoadDiscarded();
  void testThumbnailQualityDefault();
  void testThumbnailPaintAtRatio();
  void testThumbnailsFollowScreenChange();
  void testZoneMaskAtDeviceResolution();
  void testZoneMaskOverlayConfined();
  void testZoneMaskRebuildsOnScreenChange();
  void testStageAnimationFrames();
  void testStageAnimationFollowsScreenChange();
  void testIconPixmapLogicalGeometry();
  void testPageBadgeAtDeviceResolution();
};

#endif
