// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_hidpi_qt.h"

#include <ApplicationSettings.h>
#include <BasicImageView.h>
#include <HiDpi.h>
#include <ImageId.h>
#include <PageId.h>
#include <PageRange.h>
#include <PageSelectionAccessor.h>
#include <PageSelectionProvider.h>
#include <PageSequence.h>
#include <ProjectPages.h>
#include <StageListView.h>
#include <StageSequence.h>
#include <ThumbnailBase.h>
#include <ThumbnailDprTracker.h>
#include <ThumbnailPixmapCache.h>
#include <Transform.h>
#include <filters/output/PictureZoneEditor.h>
#include <filters/output/Settings.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QEvent>
#include <QFile>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPixmapCache>
#include <QPolygonF>
#include <QRandomGenerator>
#include <QScreen>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QStyleOptionGraphicsItem>
#include <QTemporaryDir>
#include <QThread>
#include <QWidget>
#include <QWindow>
#include <QtMath>
#include <QtTest/QtTest>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <limits>
#include <memory>
#include <set>
#include <vector>

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

// Names of the thumbnail files with the given quality suffix ("q500"), sorted.
QStringList thumbFiles(const QString& thumbDir, const QString& quality) {
  return QDir(thumbDir).entryList(QStringList() << ("*_" + quality + ".png"), QDir::Files, QDir::Name);
}

QByteArray sha1Of(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return QByteArray();
  }
  return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha1);
}

// Remembers the status of the last completion it received.
class ResultRecorder : public ThumbnailPixmapCache::CompletionHandler {
 public:
  void operator()(const ThumbnailLoadResult& result) override {
    ++calls;
    status = result.status();
  }

  int calls = 0;
  ThumbnailLoadResult::Status status = ThumbnailLoadResult::LOAD_FAILED;
};

// Smallest, over the four offsets (0 or 1 in x and y) of the 2x2 block grid, of the fraction of blocks inside the ROI
// whose four pixels are not all equal (within 2 levels per channel). A ratio 1 buffer upscaled 2x with nearest
// sampling has uniform blocks at its aligned offset and so scores 0 there.
double minBlockNonUniformFraction(const QImage& image, const QRect& deviceRoi) {
  const QImage rgb = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  double minFraction = 1.0;
  for (int offsetY = 0; offsetY < 2; ++offsetY) {
    for (int offsetX = 0; offsetX < 2; ++offsetX) {
      int blocks = 0;
      int nonUniform = 0;
      for (int y = deviceRoi.top() + offsetY; y + 1 <= deviceRoi.bottom(); y += 2) {
        const auto* line0 = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));
        const auto* line1 = reinterpret_cast<const QRgb*>(rgb.constScanLine(y + 1));
        for (int x = deviceRoi.left() + offsetX; x + 1 <= deviceRoi.right(); x += 2) {
          const QRgb px[4] = {line0[x], line0[x + 1], line1[x], line1[x + 1]};
          bool uniform = true;
          for (int i = 1; i < 4; ++i) {
            const int diff = std::max({std::abs(qRed(px[i]) - qRed(px[0])), std::abs(qGreen(px[i]) - qGreen(px[0])),
                                       std::abs(qBlue(px[i]) - qBlue(px[0])), std::abs(qAlpha(px[i]) - qAlpha(px[0]))});
            if (diff > 2) {
              uniform = false;
              break;
            }
          }
          ++blocks;
          if (!uniform) {
            ++nonUniform;
          }
        }
      }
      const double fraction = blocks > 0 ? static_cast<double>(nonUniform) / blocks : 0.0;
      minFraction = std::min(minFraction, fraction);
    }
  }
  return minFraction;
}


// The page of the zone mask tests: white, 300 dpi.
QImage makeWhitePage() {
  QImage page(1200, 900, QImage::Format_RGB32);
  page.fill(Qt::white);
  page.setDotsPerMeterX(11811);
  page.setDotsPerMeterY(11811);
  return page;
}

const QPointF ZONE_SQUARE_CENTER(300, 225);  // image pixels
const double ZONE_SQUARE_ANGLE = 30.0;       // degrees

// A picture mask with a white square of 200x200 image pixels, rotated by 30 degrees, on black; with empty == true
// the mask is black everywhere. The white parts are the visible ones of the overlay.
imageproc::BinaryImage makeZoneMask(const bool empty) {
  QImage image(1200, 900, QImage::Format_RGB32);
  image.fill(Qt::black);
  if (!empty) {
    QPainter painter(&image);
    painter.translate(ZONE_SQUARE_CENTER);
    painter.rotate(ZONE_SQUARE_ANGLE);
    painter.fillRect(QRectF(-100, -100, 200, 200), Qt::white);
  }
  return imageproc::BinaryImage(image);
}

// True if the image point lies inside the rotated square of makeZoneMask().
bool insideZoneSquare(const QPointF& imagePoint) {
  QTransform toLocal;
  toLocal.translate(ZONE_SQUARE_CENTER.x(), ZONE_SQUARE_CENTER.y());
  toLocal.rotate(ZONE_SQUARE_ANGLE);
  const QPointF local = toLocal.inverted().map(imagePoint);
  return qAbs(local.x()) <= 100.0 && qAbs(local.y()) <= 100.0;
}

std::unique_ptr<output::PictureZoneEditor> makeZoneEditor(const QImage& page, const bool emptyMask) {
  return std::make_unique<output::PictureZoneEditor>(
      page, ImagePixmapUnion(), makeZoneMask(emptyMask), QTransform(), QPolygonF(QRectF(page.rect())),
      PageId(ImageId(QStringLiteral("page.png"))), std::make_shared<output::Settings>());
}

// Waits until the editor sits on a screen of the given ratio and holds an up-to-date HQ pixmap and picture mask.
void waitForZoneMask(output::PictureZoneEditor& editor, const double dpr) {
  waitForHq(editor, dpr);
  if (QTest::currentTestFailed()) {
    return;
  }
  QTRY_VERIFY_WITH_TIMEOUT(editor.screenPictureMaskIsCurrent(), 10000);
}


// A page selection without pages, enough to build a StageSequence.
class EmptySelectionProvider : public PageSelectionProvider {
 public:
  PageSequence allPages() const override { return PageSequence(); }

  std::set<PageId> selectedPages() const override { return std::set<PageId>(); }

  std::vector<PageRange> selectedRanges() const override { return std::vector<PageRange>(); }
};

// Bounding box of the pixels with an alpha above 0, in device pixels (null if there are none).
QRect drawnBounds(const QPixmap& pixmap) {
  const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
  QRect bounds;
  for (int y = 0; y < image.height(); ++y) {
    const auto* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
    for (int x = 0; x < image.width(); ++x) {
      if (qAlpha(line[x]) > 0) {
        bounds = bounds.isNull() ? QRect(x, y, 1, 1) : bounds.united(QRect(x, y, 1, 1));
      }
    }
  }
  return bounds;
}

// Smallest over largest number of drawn pixels (alpha above 0) among the four quadrants of the pixmap, split at its
// centre. A ring of bubbles centred in the frame is the same in all four (about 1); a ring centred at the corner of
// the frame, as when the rect is twice too large, leaves a quadrant almost empty.
double quadrantBalance(const QPixmap& pixmap) {
  const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
  int counts[4] = {0, 0, 0, 0};
  for (int y = 0; y < image.height(); ++y) {
    const auto* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
    for (int x = 0; x < image.width(); ++x) {
      if (qAlpha(line[x]) > 0) {
        ++counts[(2 * y >= image.height() ? 2 : 0) + (2 * x >= image.width() ? 1 : 0)];
      }
    }
  }
  const int largest = *std::max_element(std::begin(counts), std::end(counts));
  const int smallest = *std::min_element(std::begin(counts), std::end(counts));
  return largest > 0 ? static_cast<double>(smallest) / largest : 0.0;
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

void TestHiDpiQt::testThumbnailEffectiveSize() {
  using Cache = ThumbnailPixmapCache;
  QCOMPARE(Cache::effectiveThumbSize(QSize(250, 250), 1.0), QSize(250, 250));
  QCOMPARE(Cache::effectiveThumbSize(QSize(250, 250), 2.0), QSize(500, 500));
  // The dialog maximum is kept and multiplied.
  QCOMPARE(Cache::effectiveThumbSize(QSize(1500, 1500), 2.0), QSize(3000, 3000));
  // Beyond the dialog maximum the side is clamped.
  QCOMPARE(Cache::effectiveThumbSize(QSize(1600, 1600), 2.0), QSize(3000, 3000));
  // An unusable ratio counts as 1.
  QCOMPARE(Cache::effectiveThumbSize(QSize(250, 250), 0.0), QSize(250, 250));
  QCOMPARE(Cache::effectiveThumbSize(QSize(250, 250), std::numeric_limits<double>::quiet_NaN()), QSize(250, 250));
  // No overflow for a huge side.
  QCOMPARE(Cache::effectiveThumbSize(QSize(INT_MAX, INT_MAX), 2.0), QSize(3000, 3000));
  QCOMPARE(Cache::MAX_EFFECTIVE_THUMB_SIDE, 3000);
}

void TestHiDpiQt::testThumbnailFilesKeyedByRatio() {
  QTemporaryDir dir(QDir::tempPath() + "/thumbs-ratio-XXXXXX");
  QVERIFY(dir.isValid());
  const QImage page = makeNoisePage(QSize(1200, 900), 3);
  QVERIFY(page.save(dir.path() + "/orig.png"));
  const ImageId id(dir.path() + "/orig.png");
  const QString thumbDir = dir.path() + "/thumbs";

  ThumbnailPixmapCache cache(thumbDir, QSize(250, 250), 40, 5);
  QCOMPARE(cache.getDevicePixelRatio(), 1.0);
  cache.setDevicePixelRatio(2.0);
  QCOMPARE(cache.getDevicePixelRatio(), 2.0);
  QCOMPARE(cache.getMaxThumbSize(), QSize(250, 250));

  cache.ensureThumbnailExists(id, page);
  const QStringList q500 = thumbFiles(thumbDir, "q500");
  QCOMPARE(q500.size(), 1);
  QVERIFY(thumbFiles(thumbDir, "q250").isEmpty());
  const QString q500Path = thumbDir + "/" + q500.first();
  QCOMPARE(QImage(q500Path).size(), QSize(500, 375));

  QPixmap pm;
  QCOMPARE(cache.loadNow(id, pm), ThumbnailPixmapCache::LOADED);
  QCOMPARE(pm.width(), 500);
  const QByteArray q500Hash = sha1Of(q500Path);
  QVERIFY(!q500Hash.isEmpty());

  // A new ratio flushes the pixmap held in memory.
  cache.setDevicePixelRatio(1.0);
  QCOMPARE(cache.loadFromCache(id, pm), ThumbnailPixmapCache::LOAD_FAILED);
  cache.ensureThumbnailExists(id, page);
  const QStringList q250 = thumbFiles(thumbDir, "q250");
  QCOMPARE(q250.size(), 1);
  // makeThumbnail() scales a 1200x900 page to 249x187, not 250x187: QImage::scaled() fits the already fitted size
  // once more. The behaviour at ratio 1 is kept as it was, so the width is checked within that pixel.
  const int q250Width = QImage(thumbDir + "/" + q250.first()).width();
  QVERIFY2(q250Width >= 249 && q250Width <= 250, qPrintable(QString::number(q250Width)));
  // The file of the other size is left alone.
  QVERIFY(QFile::exists(q500Path));
  QCOMPARE(sha1Of(q500Path), q500Hash);

  QCOMPARE(cache.loadNow(id, pm), ThumbnailPixmapCache::LOADED);
  const int loadedWidth = pm.width();
  QCOMPARE(loadedWidth, q250Width);
  // An equal ratio changes nothing, in particular it does not flush.
  cache.setDevicePixelRatio(1.0);
  pm = QPixmap();
  QCOMPARE(cache.loadFromCache(id, pm), ThumbnailPixmapCache::LOADED);
  QCOMPARE(pm.width(), loadedWidth);
}

void TestHiDpiQt::testOldThumbnailLeftInPlace() {
  QTemporaryDir dir(QDir::tempPath() + "/thumbs-old-XXXXXX");
  QVERIFY(dir.isValid());
  const QImage page = makeNoisePage(QSize(1200, 900), 3);
  QVERIFY(page.save(dir.path() + "/orig.png"));
  const ImageId id(dir.path() + "/orig.png");
  const QString thumbDir = dir.path() + "/thumbs";

  QString q200Path;
  QByteArray q200Hash;
  int fileCount = 0;
  {
    ThumbnailPixmapCache oldCache(thumbDir, QSize(200, 200), 40, 5);
    oldCache.ensureThumbnailExists(id, page);
    const QStringList q200 = thumbFiles(thumbDir, "q200");
    QCOMPARE(q200.size(), 1);
    q200Path = thumbDir + "/" + q200.first();
    q200Hash = sha1Of(q200Path);
    QVERIFY(!q200Hash.isEmpty());
    fileCount = QDir(thumbDir).entryList(QDir::Files).size();
  }

  ThumbnailPixmapCache cache(thumbDir, QSize(250, 250), 40, 5);
  cache.setDevicePixelRatio(2.0);
  QPixmap pm;
  // Only the old file exists: the request is queued and builds a new one from the original image.
  QCOMPARE(cache.loadRequest(id, pm, std::weak_ptr<ThumbnailPixmapCache::CompletionHandler>()),
           ThumbnailPixmapCache::QUEUED);
  QVERIFY(pm.isNull());
  QTRY_COMPARE_WITH_TIMEOUT(cache.loadFromCache(id, pm), ThumbnailPixmapCache::LOADED, 10000);
  QCOMPARE(pm.width(), 500);
  QCOMPARE(thumbFiles(thumbDir, "q500").size(), 1);
  QCOMPARE(sha1Of(q200Path), q200Hash);
  QVERIFY(QDir(thumbDir).entryList(QDir::Files).size() >= fileCount);
  QVERIFY(QFile::exists(q200Path));
}

void TestHiDpiQt::testStaleLoadDiscarded() {
  QTemporaryDir dir(QDir::tempPath() + "/thumbs-stale-XXXXXX");
  QVERIFY(dir.isValid());
  const QImage page = makeNoisePage(QSize(1200, 900), 3);
  QVERIFY(page.save(dir.path() + "/orig.png"));
  const ImageId id(dir.path() + "/orig.png");
  const QString thumbDir = dir.path() + "/thumbs";

  ThumbnailPixmapCache cache(thumbDir, QSize(250, 250), 40, 5);
  auto recorder = std::make_shared<ResultRecorder>();
  const std::weak_ptr<ThumbnailPixmapCache::CompletionHandler> handler(recorder);

  QPixmap pm;
  QCOMPARE(cache.loadRequest(id, pm, handler), ThumbnailPixmapCache::QUEUED);
  // Wait for the worker to finish the ratio 1 load without processing events here: the file is written after the
  // worker has read the size generation, and its result event is only delivered once we run the event loop.
  QElapsedTimer timer;
  timer.start();
  while (thumbFiles(thumbDir, "q250").isEmpty() && timer.elapsed() < 10000) {
    QThread::msleep(5);
  }
  QVERIFY(!thumbFiles(thumbDir, "q250").isEmpty());

  cache.setDevicePixelRatio(2.0);
  QTRY_COMPARE_WITH_TIMEOUT(recorder->calls, 1, 10000);
  QCOMPARE(recorder->status, ThumbnailLoadResult::REQUEST_EXPIRED);
  // The old size pixmap was not stored as LOADED.
  QCOMPARE(cache.loadFromCache(id, pm), ThumbnailPixmapCache::LOAD_FAILED);
  QVERIFY(pm.isNull());

  // A new request builds the thumbnail at the new size.
  QCOMPARE(cache.loadRequest(id, pm, handler), ThumbnailPixmapCache::QUEUED);
  QTRY_COMPARE_WITH_TIMEOUT(cache.loadFromCache(id, pm), ThumbnailPixmapCache::LOADED, 10000);
  QCOMPARE(pm.width(), 500);
}

void TestHiDpiQt::testThumbnailQualityDefault() {
  // The qt_tests settings file lives below the build tree (main.cpp checks it), so this cannot touch real settings.
  QSettings settings;
  settings.remove("settings/thumbnail_quality");
  settings.sync();
  QCOMPARE(ApplicationSettings::getInstance().getThumbnailQuality(), QSize(250, 250));
}

void TestHiDpiQt::testThumbnailPaintAtRatio() {
  QTemporaryDir dir(QDir::tempPath() + "/thumbs-paint-XXXXXX");
  QVERIFY(dir.isValid());
  const QImage page = makeNoisePage(QSize(1200, 900), 4);
  QVERIFY(page.save(dir.path() + "/orig.png"));
  const ImageId id(dir.path() + "/orig.png");

  auto cache = std::make_shared<ThumbnailPixmapCache>(dir.path() + "/thumbs", QSize(250, 250), 40, 5);
  cache->setDevicePixelRatio(2.0);
  cache->ensureThumbnailExists(id, page);
  QPixmap loaded;
  QCOMPARE(cache->loadNow(id, loaded), ThumbnailPixmapCache::LOADED);
  QCOMPARE(loaded.width(), 500);

  ThumbnailBase item(cache, QSizeF(250, 160), id, ImageTransformation(QRectF(page.rect()), Dpi(300, 300)));
  QPixmapCache::clear();
  const QStyleOptionGraphicsItem option;

  QImage target2x(500, 320, QImage::Format_ARGB32_Premultiplied);
  target2x.setDevicePixelRatio(2.0);
  target2x.fill(Qt::transparent);
  {
    QPainter painter(&target2x);
    item.paint(&painter, &option, nullptr);
  }
  QPixmap temp;
  QVERIFY(QPixmapCache::find("ThumbnailBase::tempPixmap@2.00", &temp));
  QCOMPARE(temp.devicePixelRatio(), 2.0);
  QVERIFY2(temp.width() >= qCeil(item.boundingRect().width() * 2), qPrintable(QString::number(temp.width())));
  const QRect deviceRect = QRectF(item.boundingRect().left() * 2, item.boundingRect().top() * 2,
                                  item.boundingRect().width() * 2, item.boundingRect().height() * 2)
                               .toAlignedRect()
                               .adjusted(2, 2, -2, -2);
  const double fraction = minBlockNonUniformFraction(target2x, deviceRect);
  QVERIFY2(fraction >= 0.05, qPrintable(QString("non-uniform 2x2 blocks: %1").arg(fraction)));

  QImage target1x(250, 160, QImage::Format_ARGB32_Premultiplied);
  target1x.fill(Qt::transparent);
  {
    QPainter painter(&target1x);
    item.paint(&painter, &option, nullptr);
  }
  QPixmap temp1;
  QVERIFY(QPixmapCache::find("ThumbnailBase::tempPixmap@1.00", &temp1));
  QCOMPARE(temp1.devicePixelRatio(), 1.0);
  QVERIFY(QPixmapCache::find("ThumbnailBase::tempPixmap@2.00", &temp));
  QCOMPARE(temp.devicePixelRatio(), 2.0);
}

void TestHiDpiQt::testThumbnailsFollowScreenChange() {
  QTemporaryDir dir(QDir::tempPath() + "/thumbs-follow-XXXXXX");
  QVERIFY(dir.isValid());
  const QImage page = makeNoisePage(QSize(1200, 900), 5);
  QVERIFY(page.save(dir.path() + "/orig.png"));
  const ImageId id(dir.path() + "/orig.png");
  const QString thumbDir = dir.path() + "/thumbs";
  QPixmapCache::clear();

  auto cache = std::make_shared<ThumbnailPixmapCache>(thumbDir, QSize(250, 250), 40, 5);
  QGraphicsScene scene;
  scene.addItem(new ThumbnailBase(cache, QSizeF(250, 160), id,
                                  ImageTransformation(QRectF(page.rect()), Dpi(300, 300))));  // owned by the scene
  QGraphicsView view(&scene);
  auto* tracker = new ThumbnailDprTracker(&view);  // owned by the view

  // A null cache is allowed and a cache can be set again.
  tracker->setCache(nullptr);
  tracker->setCache(cache);

  placeOn(view, screenNamed("OneX"), QSize(300, 220));
  if (QTest::currentTestFailed()) {
    return;
  }
  QCOMPARE(cache->getDevicePixelRatio(), 1.0);
  QPixmap pm;
  QTRY_COMPARE_WITH_TIMEOUT(cache->loadFromCache(id, pm), ThumbnailPixmapCache::LOADED, 10000);
  QVERIFY2(pm.width() >= 249 && pm.width() <= 250, qPrintable(QString::number(pm.width())));
  QVERIFY(!thumbFiles(thumbDir, "q250").isEmpty());

  placeOn(view, screenNamed("TwoX"), QSize(300, 220));
  if (QTest::currentTestFailed()) {
    return;
  }
  QTRY_COMPARE(cache->getDevicePixelRatio(), 2.0);
  QTRY_COMPARE_WITH_TIMEOUT(cache->loadFromCache(id, pm), ThumbnailPixmapCache::LOADED, 10000);
  QCOMPARE(pm.width(), 500);
  QVERIFY(!thumbFiles(thumbDir, "q500").isEmpty());
  QPixmap temp;
  QTRY_VERIFY_WITH_TIMEOUT(QPixmapCache::find("ThumbnailBase::tempPixmap@2.00", &temp), 10000);
  QCOMPARE(temp.devicePixelRatio(), 2.0);

  placeOn(view, screenNamed("OneX"), QSize(300, 220));
  if (QTest::currentTestFailed()) {
    return;
  }
  QTRY_COMPARE(cache->getDevicePixelRatio(), 1.0);
  QTRY_COMPARE_WITH_TIMEOUT(cache->loadFromCache(id, pm), ThumbnailPixmapCache::LOADED, 10000);
  QVERIFY2(pm.width() >= 249 && pm.width() <= 250, qPrintable(QString::number(pm.width())));
}

void TestHiDpiQt::testZoneMaskAtDeviceResolution() {
  const QImage page = makeWhitePage();
  const QRectF maskRect(0, 0, 1200, 900);

  auto editorTwo = makeZoneEditor(page, false);
  placeOn(*editorTwo, screenNamed("TwoX"), QSize(400, 300));
  if (QTest::currentTestFailed()) {
    return;
  }
  waitForZoneMask(*editorTwo, 2.0);
  if (QTest::currentTestFailed()) {
    return;
  }
  const QSize expected
      = hidpi::targetRect(maskRect, editorTwo->virtualToWidget(), editorTwo->viewport()->size(), 2.0).size();
  QVERIFY(!expected.isEmpty());
  QCOMPARE(editorTwo->screenPictureMask().devicePixelRatio(), 2.0);
  QCOMPARE(editorTwo->screenPictureMask().size(), expected);

  auto editorOne = makeZoneEditor(page, false);
  placeOn(*editorOne, screenNamed("OneX"), QSize(400, 300));
  if (QTest::currentTestFailed()) {
    return;
  }
  waitForZoneMask(*editorOne, 1.0);
  if (QTest::currentTestFailed()) {
    return;
  }
  // The arithmetic of the code before the Retina work.
  const QSize legacy = editorOne->virtualToWidget()
                           .map(maskRect)
                           .boundingRect()
                           .toRect()
                           .intersected(QRect(QPoint(0, 0), editorOne->viewport()->size()))
                           .size();
  QVERIFY(!legacy.isEmpty());
  QCOMPARE(editorOne->screenPictureMask().devicePixelRatio(), 1.0);
  QCOMPARE(editorOne->screenPictureMask().size(), legacy);
}

void TestHiDpiQt::testZoneMaskOverlayConfined() {
  const QImage page = makeWhitePage();
  auto withMask = makeZoneEditor(page, false);
  auto emptyMask = makeZoneEditor(page, true);
  for (output::PictureZoneEditor* editor : {withMask.get(), emptyMask.get()}) {
    placeOn(*editor, screenNamed("TwoX"), QSize(400, 300));
    if (QTest::currentTestFailed()) {
      return;
    }
  }
  for (output::PictureZoneEditor* editor : {withMask.get(), emptyMask.get()}) {
    waitForZoneMask(*editor, 2.0);
    if (QTest::currentTestFailed()) {
      return;
    }
  }

  const QTransform imageToWidget = withMask->imageToWidget();
  const QRect deviceRect
      = hidpi::targetRect(QRectF(0, 0, 1200, 900), withMask->virtualToWidget(), withMask->viewport()->size(), 2.0);
  const QPointF origin = hidpi::toLogical(deviceRect.topLeft(), 2.0);
  const QPointF center = imageToWidget.map(ZONE_SQUARE_CENTER);  // logical
  // Where the centre of the square would lie if the per-frame copy were drawn at twice its logical size.
  const QPointF doubled = origin + 2.0 * (center - origin);

  const QPoint onSquare = (center * 2.0).toPoint();
  const QPoint wrongPlace = (doubled * 2.0).toPoint();
  const QRect viewportDevice(QPoint(0, 0), withMask->viewport()->size() * 2);
  QVERIFY(viewportDevice.contains(onSquare));
  QVERIFY2(viewportDevice.contains(wrongPlace),
           qPrintable(QString("probe at %1,%2 outside the viewport").arg(wrongPlace.x()).arg(wrongPlace.y())));
  QVERIFY(insideZoneSquare(imageToWidget.inverted().map(QPointF(onSquare) / 2.0)));
  QVERIFY(!insideZoneSquare(imageToWidget.inverted().map(QPointF(wrongPlace) / 2.0)));

  const QPixmap grabWith = withMask->viewport()->grab();
  const QPixmap grabEmpty = emptyMask->viewport()->grab();
  QCOMPARE(grabWith.devicePixelRatio(), 2.0);
  QCOMPARE(grabWith.size(), withMask->viewport()->size() * 2);
  const QImage with = toRgb32(grabWith.toImage());
  const QImage empty = toRgb32(grabEmpty.toImage());

  auto channelDiff = [&](const QPoint& p) {
    const QRgb a = with.pixel(p);
    const QRgb b = empty.pixel(p);
    return std::max({std::abs(qRed(a) - qRed(b)), std::abs(qGreen(a) - qGreen(b)), std::abs(qBlue(a) - qBlue(b))});
  };
  QVERIFY2(channelDiff(onSquare) > 8, qPrintable(QString("tint at the square centre: %1").arg(channelDiff(onSquare))));
  QVERIFY2(channelDiff(wrongPlace) <= 2,
           qPrintable(QString("the overlay reaches %1,%2: difference %2").arg(wrongPlace.x()).arg(wrongPlace.y())));
}

void TestHiDpiQt::testZoneMaskRebuildsOnScreenChange() {
  const QImage page = makeWhitePage();
  auto editor = makeZoneEditor(page, false);

  placeOn(*editor, screenNamed("OneX"), QSize(400, 300));
  if (QTest::currentTestFailed()) {
    return;
  }
  waitForZoneMask(*editor, 1.0);
  if (QTest::currentTestFailed()) {
    return;
  }
  QCOMPARE(editor->screenPictureMask().devicePixelRatio(), 1.0);

  placeOn(*editor, screenNamed("TwoX"), QSize(400, 300));
  if (QTest::currentTestFailed()) {
    return;
  }
  waitForZoneMask(*editor, 2.0);
  if (QTest::currentTestFailed()) {
    return;
  }
  QCOMPARE(editor->screenPictureMask().devicePixelRatio(), 2.0);

  placeOn(*editor, screenNamed("OneX"), QSize(400, 300));
  if (QTest::currentTestFailed()) {
    return;
  }
  waitForZoneMask(*editor, 1.0);
  if (QTest::currentTestFailed()) {
    return;
  }
  QCOMPARE(editor->screenPictureMask().devicePixelRatio(), 1.0);
}

void TestHiDpiQt::testStageAnimationFrames() {
  const auto frames2 = StageListView::createBatchAnimationFrames(20, 2.0, Qt::black, Qt::gray);
  QCOMPARE(frames2.size(), static_cast<size_t>(8));
  for (const QPixmap& frame : frames2) {
    QCOMPARE(frame.size(), QSize(40, 40));
    QCOMPARE(frame.devicePixelRatio(), 2.0);
    QCOMPARE(hidpi::logicalSize(frame), QSizeF(20, 20));
    const QRect bounds = drawnBounds(frame);
    QVERIFY(!bounds.isNull());
    QVERIFY2(quadrantBalance(frame) >= 0.75, qPrintable(QString::number(quadrantBalance(frame))));
    // Centre of the drawn pixels in device pixels; the frame centre is at (20, 20).
    const QPointF center(bounds.left() + 0.5 * bounds.width(), bounds.top() + 0.5 * bounds.height());
    QVERIFY2(qAbs(center.x() - 20.0) <= 1.0 && qAbs(center.y() - 20.0) <= 1.0,
             qPrintable(QString("centre %1,%2").arg(center.x()).arg(center.y())));
  }

  const auto frames1 = StageListView::createBatchAnimationFrames(20, 1.0, Qt::black, Qt::gray);
  QCOMPARE(frames1.size(), static_cast<size_t>(8));
  for (const QPixmap& frame : frames1) {
    QCOMPARE(frame.size(), QSize(20, 20));
    QCOMPARE(frame.devicePixelRatio(), 1.0);
    const QRect bounds = drawnBounds(frame);
    QVERIFY(!bounds.isNull());
    QVERIFY2(quadrantBalance(frame) >= 0.75, qPrintable(QString::number(quadrantBalance(frame))));
    const QPointF center(bounds.left() + 0.5 * bounds.width(), bounds.top() + 0.5 * bounds.height());
    QVERIFY2(qAbs(center.x() - 10.0) <= 1.0 && qAbs(center.y() - 10.0) <= 1.0,
             qPrintable(QString("centre %1,%2").arg(center.x()).arg(center.y())));
  }

  // Edge probe: the smallest square the view can ask for.
  const auto tiny = StageListView::createBatchAnimationFrames(1, 2.0, Qt::black, Qt::gray);
  QCOMPARE(tiny.size(), static_cast<size_t>(8));
  for (const QPixmap& frame : tiny) {
    QCOMPARE(frame.size(), QSize(2, 2));
  }
  QCOMPARE(hidpi::logicalRect(QPixmap()), QRectF(0, 0, 0, 0));
}

void TestHiDpiQt::testStageAnimationFollowsScreenChange() {
  const auto provider = std::make_shared<EmptySelectionProvider>();
  const auto stages
      = std::make_shared<StageSequence>(std::make_shared<ProjectPages>(), PageSelectionAccessor(provider));
  StageListView view(nullptr);
  view.setStages(stages);

  placeOn(view, screenNamed("OneX"), QSize(300, 400));
  if (QTest::currentTestFailed()) {
    return;
  }
  QTRY_COMPARE(view.batchAnimationFrames().front().devicePixelRatio(), 1.0);
  QVERIFY(!view.batchAnimationFrames().empty());
  const int w1 = view.batchAnimationFrames().front().width();
  QVERIFY(w1 >= 1);

  placeOn(view, screenNamed("TwoX"), QSize(300, 400));
  if (QTest::currentTestFailed()) {
    return;
  }
  QTRY_COMPARE(view.batchAnimationFrames().front().devicePixelRatio(), 2.0);
  QCOMPARE(view.batchAnimationFrames().front().width(), 2 * w1);

  // Two events without a ratio change leave the frames alone.
  std::vector<qint64> keys;
  for (const QPixmap& frame : view.batchAnimationFrames()) {
    keys.push_back(frame.cacheKey());
  }
  QEvent event(QEvent::DevicePixelRatioChange);
  QCoreApplication::sendEvent(&view, &event);
  QCoreApplication::sendEvent(&view, &event);
  QCoreApplication::processEvents();
  QCOMPARE(view.batchAnimationFrames().size(), keys.size());
  for (size_t i = 0; i < keys.size(); ++i) {
    QCOMPARE(view.batchAnimationFrames()[i].cacheKey(), keys[i]);
  }

  placeOn(view, screenNamed("OneX"), QSize(300, 400));
  if (QTest::currentTestFailed()) {
    return;
  }
  QTRY_COMPARE(view.batchAnimationFrames().front().devicePixelRatio(), 1.0);
  QCOMPARE(view.batchAnimationFrames().front().width(), w1);
}
