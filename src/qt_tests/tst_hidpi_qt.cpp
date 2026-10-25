// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_hidpi_qt.h"

#include <BasicImageView.h>
#include <HiDpi.h>

#include <QGuiApplication>
#include <QImage>
#include <QPixmap>
#include <QRandomGenerator>
#include <QScreen>
#include <QString>
#include <QWidget>
#include <QWindow>
#include <QtTest/QtTest>

namespace {

QScreen* screenNamed(const QString& name) {
  const auto screens = QGuiApplication::screens();
  for (QScreen* screen : screens) {
    if (screen->name() == name) {
      return screen;
    }
  }
  return nullptr;
}

// Deterministic grey noise in scan order: every pixel differs from its neighbours, so any resampling difference
// shows up in a pixel comparison.
QImage makeNoisePage(const QSize& size, const quint32 seed) {
  QImage page(size, QImage::Format_RGB32);
  QRandomGenerator rng(seed);
  for (int y = 0; y < size.height(); ++y) {
    auto* line = reinterpret_cast<QRgb*>(page.scanLine(y));
    for (int x = 0; x < size.width(); ++x) {
      const int level = static_cast<int>(rng.bounded(256));
      line[x] = qRgb(level, level, level);
    }
  }
  page.setDotsPerMeterX(11811);  // 300 dpi
  page.setDotsPerMeterY(11811);
  return page;
}

// Shows the widget if needed, moves it to the screen and restores its logical size (an offscreen window keeps its
// native size on a scaled screen, so its logical size would halve). Callers check QTest::currentTestFailed().
void placeOn(QWidget& widget, QScreen* screen, const QSize& logicalSize) {
  if (widget.isHidden()) {
    widget.resize(logicalSize);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
  }
  widget.windowHandle()->setScreen(screen);
  // The offscreen platform picks the screen from the window position on every geometry change, so the window
  // has to lie inside the target screen as well.
  widget.move(screen->geometry().topLeft() + QPoint(20, 20));
  QTRY_COMPARE(widget.devicePixelRatioF(), screen->devicePixelRatio());
  widget.resize(logicalSize);
}

}  // namespace

void TestHiDpiQt::initTestCase() {
  QCOMPARE(QGuiApplication::platformName(), QString("offscreen"));
  QVERIFY2(screenNamed("OneX") != nullptr && screenNamed("TwoX") != nullptr,
           "the offscreen screens OneX and TwoX are missing: check QT_QPA_PLATFORM=offscreen:configfile=... "
           "set in src/qt_tests/main.cpp");
  QCOMPARE(screenNamed("OneX")->devicePixelRatio(), 1.0);
  QCOMPARE(screenNamed("TwoX")->devicePixelRatio(), 2.0);
}

void TestHiDpiQt::testHqPixmapAtDeviceResolution() {
  const QImage page = makeNoisePage(QSize(1200, 900), 1);
  BasicImageView view(page);
  placeOn(view, screenNamed("TwoX"), QSize(400, 300));
  if (QTest::currentTestFailed()) {
    return;
  }
  // The viewport child learns about the new ratio a little after its top-level parent.
  QTRY_COMPARE(view.viewport()->devicePixelRatioF(), 2.0);
  QTRY_VERIFY_WITH_TIMEOUT(view.hqPixmapIsCurrent(), 10000);

  const QRect expected = hidpi::targetRect(QRectF(page.rect()), view.imageToWidget(), view.viewport()->size(), 2.0);
  QVERIFY(!expected.isEmpty());
  QCOMPARE(view.hqPixmap().devicePixelRatio(), 2.0);
  QCOMPARE(view.hqPixmap().size(), expected.size());
  QCOMPARE(view.hqPixmapPos(), QPointF(expected.topLeft()) / 2.0);
}

void TestHiDpiQt::testHqPixmapAtOneXUnchanged() {
  const QImage page = makeNoisePage(QSize(1200, 900), 1);
  BasicImageView view(page);
  placeOn(view, screenNamed("OneX"), QSize(400, 300));
  if (QTest::currentTestFailed()) {
    return;
  }
  QTRY_VERIFY_WITH_TIMEOUT(view.hqPixmapIsCurrent(), 10000);
  QCOMPARE(view.viewport()->devicePixelRatioF(), 1.0);

  // The arithmetic of the code before the Retina work.
  const QRect legacy = view.imageToWidget()
                           .map(QRectF(page.rect()))
                           .boundingRect()
                           .toRect()
                           .intersected(QRect(QPoint(0, 0), view.viewport()->size()));
  QVERIFY(!legacy.isEmpty());
  QCOMPARE(view.hqPixmap().devicePixelRatio(), 1.0);
  QCOMPARE(view.hqPixmap().size(), legacy.size());
  QCOMPARE(view.hqPixmapPos(), QPointF(legacy.topLeft()));
}
