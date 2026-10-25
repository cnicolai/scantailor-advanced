// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_hidpi_qt.h"

#include <BasicImageView.h>
#include <HiDpi.h>
#include <Transform.h>

#include <QCoreApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QImage>
#include <QPixmap>
#include <QRandomGenerator>
#include <QScreen>
#include <QString>
#include <QWidget>
#include <QWindow>
#include <QtTest/QtTest>
#include <algorithm>
#include <cstdlib>
#include <limits>

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

// Waits until the viewport of the view has the given ratio (the child learns about a new ratio a little after its
// top-level parent) and the view holds an up-to-date HQ pixmap.
void waitForHq(ImageViewBase& view, const double dpr) {
  QTRY_COMPARE(view.viewport()->devicePixelRatioF(), dpr);
  QTRY_VERIFY_WITH_TIMEOUT(view.hqPixmapIsCurrent(), 10000);
}

QImage toRgb32(const QImage& image) {
  QImage result = image.convertToFormat(QImage::Format_RGB32);
  result.setDevicePixelRatio(1.0);
  return result;
}

struct Comparison {
  double equalFraction = 0.0;
  int maxDiff = 0;
  int pixels = 0;
};

// Compares the pixels of a region of the grab with the reference, whose pixel (0, 0) lies at refOrigin.
Comparison compareRegion(const QImage& grab, const QImage& ref, const QPoint& refOrigin, const QRect& region) {
  Comparison result;
  int equal = 0;
  for (int y = region.top(); y <= region.bottom(); ++y) {
    const auto* grabLine = reinterpret_cast<const QRgb*>(grab.constScanLine(y));
    const auto* refLine = reinterpret_cast<const QRgb*>(ref.constScanLine(y - refOrigin.y()));
    for (int x = region.left(); x <= region.right(); ++x) {
      const QRgb a = grabLine[x];
      const QRgb b = refLine[x - refOrigin.x()];
      const int diff
          = std::max({std::abs(qRed(a) - qRed(b)), std::abs(qGreen(a) - qGreen(b)), std::abs(qBlue(a) - qBlue(b))});
      result.maxDiff = std::max(result.maxDiff, diff);
      if (diff == 0) {
        ++equal;
      }
      ++result.pixels;
    }
  }
  result.equalFraction = result.pixels > 0 ? static_cast<double>(equal) / result.pixels : 0.0;
  return result;
}

// Counts DevicePixelRatioChange events delivered to the object it is installed on.
class RatioChangeCounter : public QObject {
 public:
  int count() const { return m_count; }

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override {
    if (event->type() == QEvent::DevicePixelRatioChange) {
      ++m_count;
    }
    return QObject::eventFilter(watched, event);
  }

 private:
  int m_count = 0;
};

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
  waitForHq(view, 2.0);
  if (QTest::currentTestFailed()) {
    return;
  }

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
  waitForHq(view, 1.0);
  if (QTest::currentTestFailed()) {
    return;
  }

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

void TestHiDpiQt::testHelpersMatchLegacyAtRatio1() {
  const QRectF rect(0, 0, 1200, 900);
  const QSize viewport(400, 300);
  const QTransform transforms[] = {
      QTransform(),
      QTransform::fromScale(0.37, 0.37),
      QTransform::fromScale(2, 2) * QTransform::fromTranslate(13.5, -7.25),
      QTransform().rotate(1.5) * QTransform::fromScale(0.5, 0.5),
  };
  for (const QTransform& x : transforms) {
    const QRect legacy = x.map(rect).boundingRect().toRect().intersected(QRect(QPoint(0, 0), viewport));
    QCOMPARE(hidpi::targetRect(rect, x, viewport, 1.0), legacy);
    QCOMPARE(hidpi::toDevice(x, 1.0).map(QPointF(123.25, 45.5)), x.map(QPointF(123.25, 45.5)));
  }
  QCOMPARE(hidpi::deviceSize(QSize(400, 300), 1.0), QSize(400, 300));
  QCOMPARE(hidpi::toLogical(QPoint(7, 9), 1.0), QPointF(7, 9));

  const QPixmap pixmap = hidpi::makePixmap(QSize(16, 16), 1.0);
  QCOMPARE(pixmap.size(), QSize(16, 16));
  QCOMPARE(pixmap.devicePixelRatio(), 1.0);
}

void TestHiDpiQt::testHelpersAtFractionalRatio() {
  QCOMPARE(hidpi::deviceSize(QSize(15, 15), 1.5), QSize(23, 23));
  QCOMPARE(hidpi::deviceSize(QSize(400, 300), 2.0), QSize(800, 600));
  QCOMPARE(hidpi::deviceSize(QSize(0, 0), 2.0), QSize(0, 0));
  QCOMPARE(hidpi::deviceSize(QSize(1, 1), 1.5), QSize(2, 2));
  QCOMPARE(hidpi::deviceSize(QSize(1, 1), 0.25), QSize(1, 1));
  QCOMPARE(hidpi::toLogical(QPoint(3, 3), 2.0), QPointF(1.5, 1.5));

  QCOMPARE(hidpi::sanitizedRatio(0.0), 1.0);
  QCOMPARE(hidpi::sanitizedRatio(-2.0), 1.0);
  QCOMPARE(hidpi::sanitizedRatio(std::numeric_limits<double>::quiet_NaN()), 1.0);
  QCOMPARE(hidpi::sanitizedRatio(std::numeric_limits<double>::infinity()), 1.0);
  QCOMPARE(hidpi::sanitizedRatio(2.0), 2.0);

  QVERIFY(hidpi::targetRect(QRectF(0, 0, 100, 100), QTransform(), QSize(0, 0), 2.0).isEmpty());

  const QPixmap pixmap = hidpi::makePixmap(QSize(16, 16), 2.0);
  QCOMPARE(pixmap.size(), QSize(32, 32));
  QCOMPARE(pixmap.devicePixelRatio(), 2.0);
  QCOMPARE(hidpi::logicalSize(pixmap), QSizeF(16, 16));
  QCOMPARE(hidpi::logicalRect(pixmap), QRectF(0, 0, 16, 16));
  QCOMPARE(hidpi::logicalRect(QPixmap()), QRectF(0, 0, 0, 0));
}

void TestHiDpiQt::testHqGrabMatchesDeviceReference() {
  const QImage page = makeNoisePage(QSize(1200, 900), 2);
  BasicImageView view(page);
  placeOn(view, screenNamed("TwoX"), QSize(400, 300));
  if (QTest::currentTestFailed()) {
    return;
  }
  waitForHq(view, 2.0);
  if (QTest::currentTestFailed()) {
    return;
  }

  const double fit = view.virtualToWidget().m11();
  QVERIFY(fit > 0.0);
  struct ZoomCase {
    const char* name;
    double zoom;
  };
  const ZoomCase cases[] = {{"fit", 1.0}, {"1:1", 1.0 / fit}, {"2:1", 2.0 / fit}};

  for (const ZoomCase& zoomCase : cases) {
    view.setZoomLevel(zoomCase.zoom);
    waitForHq(view, 2.0);
    if (QTest::currentTestFailed()) {
      return;
    }

    const QPixmap grabbed = view.viewport()->grab();
    QCOMPARE(grabbed.devicePixelRatio(), 2.0);
    QCOMPARE(grabbed.size(), view.viewport()->size() * 2);
    const QImage grab = toRgb32(grabbed.toImage());

    const QRect dev = hidpi::targetRect(QRectF(page.rect()), view.imageToWidget(), view.viewport()->size(), 2.0);
    QVERIFY(!dev.isEmpty());
    const QImage ref
        = toRgb32(imageproc::transform(page, hidpi::toDevice(view.imageToWidget(), 2.0), dev,
                                       imageproc::OutsidePixels::assumeWeakColor(Qt::white), QSizeF(0.0, 0.0)));
    const QRect region = dev.adjusted(3, 3, -3, -3);
    QVERIFY(!region.isEmpty());

    const Comparison cmp = compareRegion(grab, ref, dev.topLeft(), region);
    QVERIFY2(cmp.equalFraction >= 0.999 && cmp.maxDiff <= 2,
             qPrintable(QString("zoom %1: %2 of %3 pixels equal, maximum channel difference %4")
                            .arg(zoomCase.name)
                            .arg(cmp.equalFraction)
                            .arg(cmp.pixels)
                            .arg(cmp.maxDiff)));

    if (zoomCase.zoom == 1.0) {
      // Counterfactual: a buffer built at ratio 1 and scaled up must not pass the comparison above.
      const QRect logical = hidpi::targetRect(QRectF(page.rect()), view.imageToWidget(), view.viewport()->size(), 1.0);
      const QImage cf
          = toRgb32(imageproc::transform(page, view.imageToWidget(), logical,
                                         imageproc::OutsidePixels::assumeWeakColor(Qt::white), QSizeF(0.0, 0.0)))
                .scaled(logical.size() * 2, Qt::IgnoreAspectRatio, Qt::FastTransformation);
      const QPoint cfOrigin = logical.topLeft() * 2;
      const QRect cfRegion = region.intersected(QRect(cfOrigin, cf.size()));
      QVERIFY(!cfRegion.isEmpty());
      int different = 0;
      int total = 0;
      for (int y = cfRegion.top(); y <= cfRegion.bottom(); ++y) {
        const auto* cfLine = reinterpret_cast<const QRgb*>(cf.constScanLine(y - cfOrigin.y()));
        const auto* refLine = reinterpret_cast<const QRgb*>(ref.constScanLine(y - dev.top()));
        for (int x = cfRegion.left(); x <= cfRegion.right(); ++x) {
          const QRgb a = cfLine[x - cfOrigin.x()];
          const QRgb b = refLine[x - dev.left()];
          if (std::max({std::abs(qRed(a) - qRed(b)), std::abs(qGreen(a) - qGreen(b)), std::abs(qBlue(a) - qBlue(b))})
              > 2) {
            ++different;
          }
          ++total;
        }
      }
      const double differentFraction = static_cast<double>(different) / total;
      QVERIFY2(
          differentFraction >= 0.05,
          qPrintable(
              QString("a ratio 1 buffer differs from the reference in only %1 of the pixels").arg(differentFraction)));
    }
  }
}

void TestHiDpiQt::testHqRebuildsOnScreenChange() {
  const QImage page = makeNoisePage(QSize(1200, 900), 3);
  BasicImageView view(page);
  RatioChangeCounter counter;
  view.installEventFilter(&counter);

  placeOn(view, screenNamed("OneX"), QSize(400, 300));
  if (QTest::currentTestFailed()) {
    return;
  }
  waitForHq(view, 1.0);
  if (QTest::currentTestFailed()) {
    return;
  }
  QCOMPARE(view.hqPixmap().devicePixelRatio(), 1.0);

  placeOn(view, screenNamed("TwoX"), QSize(400, 300));
  if (QTest::currentTestFailed()) {
    return;
  }
  waitForHq(view, 2.0);
  if (QTest::currentTestFailed()) {
    return;
  }
  QCOMPARE(view.hqPixmap().devicePixelRatio(), 2.0);
  QCOMPARE(view.hqPixmap().size(),
           hidpi::targetRect(QRectF(page.rect()), view.imageToWidget(), view.viewport()->size(), 2.0).size());
  QVERIFY(counter.count() >= 1);

  placeOn(view, screenNamed("OneX"), QSize(400, 300));
  if (QTest::currentTestFailed()) {
    return;
  }
  waitForHq(view, 1.0);
  if (QTest::currentTestFailed()) {
    return;
  }
  QCOMPARE(view.hqPixmap().devicePixelRatio(), 1.0);
}

void TestHiDpiQt::testRepeatedRatioEventKeepsHqPixmap() {
  const QImage page = makeNoisePage(QSize(1200, 900), 4);
  BasicImageView view(page);
  placeOn(view, screenNamed("TwoX"), QSize(400, 300));
  if (QTest::currentTestFailed()) {
    return;
  }
  waitForHq(view, 2.0);
  if (QTest::currentTestFailed()) {
    return;
  }
  const qint64 key = view.hqPixmap().cacheKey();

  QEvent event(QEvent::DevicePixelRatioChange);
  QCoreApplication::sendEvent(&view, &event);
  QCoreApplication::sendEvent(&view, &event);
  QCoreApplication::processEvents();

  QVERIFY(view.hqPixmapIsCurrent());
  QCOMPARE(view.hqPixmap().cacheKey(), key);
}

void TestHiDpiQt::testSmoothingSwitchPhysicalSize() {
  const QImage page = makeNoisePage(QSize(1200, 900), 6);
  BasicImageView view(page);
  const char* const names[] = {"OneX", "TwoX"};
  for (const char* name : names) {
    placeOn(view, screenNamed(name), QSize(400, 300));
    if (QTest::currentTestFailed()) {
      return;
    }
    waitForHq(view, screenNamed(name)->devicePixelRatio());
    if (QTest::currentTestFailed()) {
      return;
    }

    const QScreen* screen = view.screen();
    QVERIFY(screen != nullptr);
    QCOMPARE(screen->name(), QString(name));
    const double expected = view.virtualToWidget().m11() * screen->physicalSize().width() / screen->geometry().width();
    const double actual = view.displayedSourcePixelWidthMM();
    QVERIFY2(expected > 0.0 && qAbs(actual / expected - 1.0) <= 0.02,
             qPrintable(QString("screen %1: displayed source pixel width %2 mm, expected %3 mm")
                            .arg(name)
                            .arg(actual)
                            .arg(expected)));
  }
}
