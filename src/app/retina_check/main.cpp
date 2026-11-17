// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
//
// Live Retina check: shows a page view on the default Cocoa platform, records the device pixel
// ratio of screen, window and view, compares the page view with a device-resolution reference of the same synthetic
// page at fit, 1:1 and 2:1 zoom, and measures whether the 0.5 mm smoothing switch counts physical millimetres.
// Further sections measure the thumbnail column and its cache, the picture-zone mask, page badges, deskew handles,
// the stage-list animation, a list decoration like the relinking dialog's, and the colour picker's grab.
// usage: retina_check <output-dir> [--section page|thumbnails|overlays|picker|all]
// The output directory must lie below the checkout's build/ directory. QSettings and temporary files are kept
// inside it, so a run never touches the user's real settings. The tool runs on the platform Qt picks by default
// (cocoa) and exits with 3 on any other platform, so an offscreen or scaled run cannot pass for a Retina run.

#include <BasicImageView.h>
#include <BinaryImage.h>
#include <Dpi.h>
#include <HiDpi.h>
#include <ImageId.h>
#include <ImageTransformation.h>
#include <PageGenerator.h>
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
#include <core/Application.h>
#include <core/ApplicationSettings.h>
#include <core/ColorSchemeFactory.h>
#include <core/ColorSchemeManager.h>
#include <core/IconProvider.h>
#include <core/StyledIconPack.h>
#include <filters/deskew/ImageView.h>
#include <filters/output/ColorPickupInteraction.h>
#include <filters/output/PictureZoneEditor.h>
#include <filters/output/Settings.h>

#include <QAbstractItemModel>
#include <QByteArray>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QListView>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPalette>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QPolygonF>
#include <QRect>
#include <QRectF>
#include <QRegularExpression>
#include <QScreen>
#include <QSettings>
#include <QSize>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QTransform>
#include <QWidget>
#include <QWindow>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <set>
#include <vector>

#include "HarnessGuards.h"

namespace {

const char* const kUsage = "usage: retina_check <output-dir> [--section page|thumbnails|overlays|picker|all]\n";

// Collects the "key: value" result lines, echoes them to stdout and writes them to summary.txt.
class Summary {
 public:
  void add(const QString& key, const QString& value) {
    const QString line = key + ": " + value;
    m_lines.push_back(line);
    std::printf("%s\n", line.toLocal8Bit().constData());
    std::fflush(stdout);
  }

  bool save(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      return false;
    }
    QTextStream out(&file);
    for (const QString& line : m_lines) {
      out << line << "\n";
    }
    return true;
  }

 private:
  QStringList m_lines;
};

QString number(const double value) {
  return QString::number(value);
}

QString fixed(const double value, const int decimals) {
  return QString::number(value, 'f', decimals);
}

QString sizeText(const QSize& size) {
  return QString("%1x%2").arg(size.width()).arg(size.height());
}

// Processes events until the condition holds or the limit in milliseconds has passed.
template <typename Condition>
bool waitFor(const Condition& condition, const int limitMs) {
  QElapsedTimer timer;
  timer.start();
  while (!condition()) {
    if (timer.elapsed() > limitMs) {
      return false;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    QThread::msleep(10);
  }
  return true;
}

QImage toRgb32(const QImage& image) {
  QImage result = image.convertToFormat(QImage::Format_RGB32);
  result.setDevicePixelRatio(1.0);
  return result;
}

int channelDiff(const QRgb a, const QRgb b) {
  return std::max({std::abs(qRed(a) - qRed(b)), std::abs(qGreen(a) - qGreen(b)), std::abs(qBlue(a) - qBlue(b))});
}

struct Comparison {
  double equalFraction = 0.0;
  int maxDiff = 0;
};

// Compares the pixels of a region of the grab with the reference, whose pixel (0, 0) lies at refOrigin.
Comparison compareRegion(const QImage& grab, const QImage& ref, const QPoint& refOrigin, const QRect& region) {
  Comparison result;
  int equal = 0;
  int total = 0;
  for (int y = region.top(); y <= region.bottom(); ++y) {
    const auto* grabLine = reinterpret_cast<const QRgb*>(grab.constScanLine(y));
    const auto* refLine = reinterpret_cast<const QRgb*>(ref.constScanLine(y - refOrigin.y()));
    for (int x = region.left(); x <= region.right(); ++x) {
      const int diff = channelDiff(grabLine[x], refLine[x - refOrigin.x()]);
      result.maxDiff = std::max(result.maxDiff, diff);
      if (diff == 0) {
        ++equal;
      }
      ++total;
    }
  }
  result.equalFraction = total > 0 ? static_cast<double>(equal) / total : 0.0;
  return result;
}

// The share of pixels of the region in which a ratio 1 rendering scaled up by 2 differs from the reference.
double counterfactualDiffFraction(const QImage& page,
                                  const QTransform& imageToWidget,
                                  const QSize& viewport,
                                  const QImage& ref,
                                  const QRect& refRect,
                                  const QRect& region) {
  const QRect logical = hidpi::targetRect(QRectF(page.rect()), imageToWidget, viewport, 1.0);
  if (logical.isEmpty()) {
    return 0.0;
  }
  const QImage cf
      = toRgb32(imageproc::transform(page, imageToWidget, logical, imageproc::OutsidePixels::assumeWeakColor(Qt::white),
                                     QSizeF(0.0, 0.0)))
            .scaled(logical.size() * 2, Qt::IgnoreAspectRatio, Qt::FastTransformation);
  const QPoint cfOrigin = logical.topLeft() * 2;
  const QRect cfRegion = region.intersected(QRect(cfOrigin, cf.size()));
  if (cfRegion.isEmpty()) {
    return 0.0;
  }
  int different = 0;
  int total = 0;
  for (int y = cfRegion.top(); y <= cfRegion.bottom(); ++y) {
    const auto* cfLine = reinterpret_cast<const QRgb*>(cf.constScanLine(y - cfOrigin.y()));
    const auto* refLine = reinterpret_cast<const QRgb*>(ref.constScanLine(y - refRect.top()));
    for (int x = cfRegion.left(); x <= cfRegion.right(); ++x) {
      if (channelDiff(cfLine[x - cfOrigin.x()], refLine[x - refRect.left()]) > 2) {
        ++different;
      }
      ++total;
    }
  }
  return static_cast<double>(different) / total;
}

// A 256x256 (or smaller) rectangle centred in the region.
QRect cropRect(const QRect& region) {
  const int w = std::min(256, region.width());
  const int h = std::min(256, region.height());
  return QRect(region.left() + (region.width() - w) / 2, region.top() + (region.height() - h) / 2, w, h);
}

bool savePng(const QImage& image, const QString& path) {
  if (!image.save(path, "PNG")) {
    std::fprintf(stderr, "retina_check: cannot write %s\n", path.toLocal8Bit().constData());
    return false;
  }
  return true;
}

// Measures one zoom of the page view; returns true if it passes.
bool measureZoom(Summary& summary,
                 BasicImageView& view,
                 const QImage& page,
                 const QString& name,
                 const double windowDpr,
                 const QString& outDir) {
  const QString prefix = "page_" + name + "_";
  if (!waitFor([&view] { return view.hqPixmapIsCurrent(); }, 15000)) {
    summary.add(prefix + "reason", "hq-timeout");
    summary.add(prefix + "verdict", "FAIL");
    return false;
  }

  const double dpr = view.viewport()->devicePixelRatioF();
  const QTransform imageToWidget = view.imageToWidget();
  const QSize viewport = view.viewport()->size();
  const QRect expected = hidpi::targetRect(QRectF(page.rect()), imageToWidget, viewport, dpr);
  const double hqDpr = view.hqPixmap().devicePixelRatio();
  const QSize hqSize = view.hqPixmap().size();

  summary.add(prefix + "hq_dpr", number(hqDpr));
  summary.add(prefix + "hq_size", sizeText(hqSize));
  summary.add(prefix + "expected_size", sizeText(expected.size()));

  const QImage grab = toRgb32(view.viewport()->grab().toImage());
  const QImage ref
      = toRgb32(imageproc::transform(page, hidpi::toDevice(imageToWidget, dpr), expected,
                                     imageproc::OutsidePixels::assumeWeakColor(Qt::white), QSizeF(0.0, 0.0)));
  const QRect region = expected.adjusted(3, 3, -3, -3);
  if (expected.isEmpty() || region.isEmpty() || !grab.rect().contains(expected) || ref.size() != expected.size()) {
    summary.add(prefix + "reason", "empty-or-out-of-bounds-target");
    summary.add(prefix + "verdict", "FAIL");
    return false;
  }

  const Comparison cmp = compareRegion(grab, ref, expected.topLeft(), region);
  summary.add(prefix + "equal_fraction", fixed(cmp.equalFraction, 6));
  summary.add(prefix + "max_diff", QString::number(cmp.maxDiff));

  const double counterfactual
      = counterfactualDiffFraction(page, imageToWidget, viewport, ref, QRect(expected.topLeft(), ref.size()), region);
  summary.add(prefix + "counterfactual_diff_fraction", fixed(counterfactual, 6));

  const QRect crop = cropRect(region);
  const bool saved
      = savePng(grab.copy(crop), outDir + "/page-" + name + "-grab.png")
        && savePng(ref.copy(crop.translated(-expected.topLeft())), outDir + "/page-" + name + "-reference.png");

  bool pass = saved && windowDpr >= 2.0 && hqDpr == dpr && hqSize == expected.size() && cmp.equalFraction >= 0.999
              && cmp.maxDiff <= 2;
  // At 1:1 and 2:1 a sharp device rendering of an axis-aligned page equals pixel replication, so the counterfactual is
  // required at fit only.
  if (name == "fit") {
    pass = pass && counterfactual >= 0.05;
  }
  summary.add(prefix + "verdict", pass ? "PASS" : "FAIL");
  return pass;
}

// Returns true if the smoothing check passes. Called at fit after the HQ pixmap is current.
bool measureSmoothing(Summary& summary, const BasicImageView& view, const double screenMmPerLogicalPx) {
  const double sourcePixelMm = view.displayedSourcePixelWidthMM();
  const double referenceMm = view.virtualToWidget().m11() * screenMmPerLogicalPx;
  const double ratio = referenceMm > 0.0 ? sourcePixelMm / referenceMm : 0.0;
  summary.add("smoothing_fit_source_px_mm", fixed(sourcePixelMm, 6));
  summary.add("smoothing_fit_reference_mm", fixed(referenceMm, 6));
  summary.add("smoothing_ratio", fixed(ratio, 4));
  // The zoom (logical pixels per source pixel) at which a source pixel is 0.5 mm wide on this screen.
  summary.add("smoothing_switch_xscale", screenMmPerLogicalPx > 0.0 ? fixed(0.5 / screenMmPerLogicalPx, 4) : "0");
  const bool pass = ratio >= 0.98 && ratio <= 1.02;
  summary.add("smoothing_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

bool runPageSection(Summary& summary, const QString& outDir) {
  const QImage page = test_support::generatePage({test_support::PageKind::FLAT, 300, 1, 0.0}).image;
  if (page.isNull()) {
    summary.add("page_verdict", "FAIL");
    summary.add("page_reason", "no-page");
    return false;
  }

  BasicImageView view(page);
  view.resize(1280, 900);
  view.show();
  view.raise();
  if (!waitFor([&view] { return view.windowHandle() && view.windowHandle()->isExposed(); }, 5000)) {
    summary.add("page_verdict", "FAIL");
    summary.add("page_reason", "not-exposed");
    return false;
  }

  const QScreen* screen = view.screen();
  const double windowDpr = view.windowHandle()->devicePixelRatio();
  const double screenMmPerLogicalPx
      = screen->geometry().width() > 0 ? screen->physicalSize().width() / screen->geometry().width() : 0.0;

  summary.add("screen_name", screen->name());
  summary.add("screen_dpr", number(screen->devicePixelRatio()));
  summary.add("screen_logical_dpi", number(screen->logicalDotsPerInch()));
  summary.add("screen_physical_dpi", number(screen->physicalDotsPerInch()));
  summary.add("screen_geometry", QString("%1,%2 %3x%4")
                                     .arg(screen->geometry().x())
                                     .arg(screen->geometry().y())
                                     .arg(screen->geometry().width())
                                     .arg(screen->geometry().height()));
  summary.add("window_dpr", number(windowDpr));
  summary.add("view_dpr", number(view.devicePixelRatioF()));
  summary.add("viewport_dpr", number(view.viewport()->devicePixelRatioF()));
  summary.add("widthMM", QString::number(view.widthMM()));
  summary.add("width", QString::number(view.width()));
  summary.add("mm_per_logical_px", fixed(static_cast<double>(view.widthMM()) / view.width(), 4));
  summary.add(
      "screen_physical_size_mm",
      QString("%1x%2").arg(screen->physicalSize().width(), 0, 'f', 1).arg(screen->physicalSize().height(), 0, 'f', 1));
  summary.add("screen_mm_per_logical_px", fixed(screenMmPerLogicalPx, 4));

  view.setZoomLevel(1.0);
  const double fit = view.virtualToWidget().m11();
  if (!(fit > 0.0)) {
    summary.add("page_verdict", "FAIL");
    summary.add("page_reason", "no-fit-scale");
    return false;
  }

  bool allPass = true;
  bool smoothingPass = false;
  allPass = measureZoom(summary, view, page, "fit", windowDpr, outDir) && allPass;
  smoothingPass = measureSmoothing(summary, view, screenMmPerLogicalPx);

  view.setZoomLevel(1.0 / fit);
  allPass = measureZoom(summary, view, page, "100", windowDpr, outDir) && allPass;

  view.setZoomLevel(2.0 / fit);
  allPass = measureZoom(summary, view, page, "200", windowDpr, outDir) && allPass;

  const bool pass = allPass && smoothingPass;
  summary.add("page_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

// ---- Further sections -------------------------------------------------------------------------------------------

// Processes events for the given time.
void pump(const int ms) {
  QElapsedTimer timer;
  timer.start();
  while (timer.elapsed() < ms) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    QThread::msleep(10);
  }
}

// Shows the widget as a top-level window and waits until its window is exposed.
bool showAndExpose(QWidget& widget, const QSize& logicalSize) {
  widget.resize(logicalSize);
  widget.show();
  widget.raise();
  if (!waitFor([&widget] { return widget.windowHandle() && widget.windowHandle()->isExposed(); }, 5000)) {
    return false;
  }
  pump(150);
  return true;
}

bool failElement(Summary& summary, const QString& prefix, const QString& reason) {
  summary.add(prefix + "reason", reason);
  summary.add(prefix + "verdict", "FAIL");
  return false;
}

// Smallest, over the four offsets (0 or 1 in x and y) of the 2x2 block grid, of the fraction of blocks inside the ROI
// whose four pixels are not all equal (within 2 levels per channel). A ratio 1 buffer upscaled 2x with nearest
// sampling has uniform blocks at its aligned offset and so scores 0 there. With a filter, only blocks whose centre
// (the corner shared by the four pixels) the filter accepts take part. This is the measure of
// minBlockNonUniformFraction in src/qt_tests/tst_hidpi_qt.cpp.
double minBlockNonUniformFraction(const QImage& image,
                                  const QRect& deviceRoi,
                                  const std::function<bool(const QPointF&)>& accept = nullptr) {
  const QRect roi = deviceRoi.intersected(image.rect());
  const QImage rgb = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  double minFraction = 1.0;
  for (int offsetY = 0; offsetY < 2; ++offsetY) {
    for (int offsetX = 0; offsetX < 2; ++offsetX) {
      int blocks = 0;
      int nonUniform = 0;
      for (int y = roi.top() + offsetY; y + 1 <= roi.bottom(); y += 2) {
        const auto* line0 = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));
        const auto* line1 = reinterpret_cast<const QRgb*>(rgb.constScanLine(y + 1));
        for (int x = roi.left() + offsetX; x + 1 <= roi.right(); x += 2) {
          if (accept && !accept(QPointF(x + 1.0, y + 1.0))) {
            continue;
          }
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

// Bounding box of the pixels inside the region that differ from the reference colour by more than the threshold in
// some channel (a null rectangle if there are none).
QRect inkBox(const QImage& image, const QRect& regionIn, const QRgb reference, const int threshold) {
  const QRect region = regionIn.intersected(image.rect());
  QRect box;
  for (int y = region.top(); y <= region.bottom(); ++y) {
    const auto* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
    for (int x = region.left(); x <= region.right(); ++x) {
      if (channelDiff(line[x], reference) > threshold) {
        box = box.isNull() ? QRect(x, y, 1, 1) : box.united(QRect(x, y, 1, 1));
      }
    }
  }
  return box;
}

QRect deviceRect(const QRectF& logical, const double dpr) {
  return QRectF(logical.x() * dpr, logical.y() * dpr, logical.width() * dpr, logical.height() * dpr).toAlignedRect();
}

// Saves a crop of at most 256x256 device pixels centred on the centre of the region.
bool saveRegionCrop(const QImage& grab, const QRect& region, const QString& path) {
  const QRect clipped = region.intersected(grab.rect());
  if (clipped.isEmpty()) {
    return false;
  }
  return savePng(grab.copy(cropRect(clipped)), path);
}

QString hexColor(const QRgb rgb) {
  return QColor(rgb).name();
}

bool writeBytes(const QString& path, const QByteArray& content) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }
  return file.write(content) == content.size();
}

// Puts the default icon pack back when the scope ends.
class DefaultIconPackRestorer {
 public:
  ~DefaultIconPackRestorer() { IconProvider::getInstance().setIconPack(StyledIconPack::createDefault()); }
};

// A page selection without pages, enough to build a StageSequence.
class EmptySelectionProvider : public PageSelectionProvider {
 public:
  PageSequence allPages() const override { return PageSequence(); }

  std::set<PageId> selectedPages() const override { return std::set<PageId>(); }

  std::vector<PageRange> selectedRanges() const override { return std::vector<PageRange>(); }
};

// ---- thumbnails -------------------------------------------------------------------------------------------------

bool runThumbnailsSection(Summary& summary, const QString& out) {
  const QString dir = out + "/thumbs-run";
  QDir(dir).removeRecursively();
  if (!QDir().mkpath(dir)) {
    return failElement(summary, "thumbnails_", "cannot-create-dir");
  }
  const QImage page = test_support::generatePage({test_support::PageKind::FLAT, 300, 1, 0.0}).image;
  if (page.isNull() || !page.save(dir + "/page.png")) {
    return failElement(summary, "thumbnails_", "no-page");
  }
  const ImageId id(dir + "/page.png");

  // As MainWindow does: the tracker belongs to the view from the start, the cache comes later and is handed over.
  QGraphicsScene scene;
  QGraphicsView view(&scene);
  auto* tracker = new ThumbnailDprTracker(&view);  // owned by the view
  if (!showAndExpose(view, QSize(400, 300))) {
    return failElement(summary, "thumbnails_", "not-exposed");
  }
  auto cache = std::make_shared<ThumbnailPixmapCache>(dir + "/thumbs", QSize(250, 250), 40, 5);
  tracker->setCache(cache);
  auto* item = new ThumbnailBase(cache, QSizeF(250, 160), id, ImageTransformation(QRectF(page.rect()), Dpi(300, 300)));
  scene.addItem(item);  // owned by the scene

  QPixmap pixmap;
  const bool loaded = waitFor([&] { return cache->loadFromCache(id, pixmap) == ThumbnailPixmapCache::LOADED; }, 15000);
  pump(300);
  const double cacheDpr = cache->getDevicePixelRatio();
  summary.add("thumb_cache_dpr", number(cacheDpr));
  summary.add("thumb_loaded", loaded ? "yes" : "no");

  QStringList suffixes;
  const QStringList files = QDir(dir + "/thumbs").entryList(QStringList() << "*.png", QDir::Files, QDir::Name);
  const QRegularExpression suffixPattern("_(q\\d+)\\.png$");
  for (const QString& file : files) {
    const QRegularExpressionMatch match = suffixPattern.match(file);
    suffixes << (match.hasMatch() ? match.captured(1) : QString("other"));
  }
  const QString suffix = suffixes.join(",");
  summary.add("thumb_file_suffix", suffix);
  summary.add("thumb_pixmap_long_side", QString::number(std::max(pixmap.width(), pixmap.height())));

  const double dpr = view.viewport()->devicePixelRatioF();
  const QPixmap grabbed = view.viewport()->grab();
  const QImage grab = toRgb32(grabbed.toImage());
  summary.add("thumb_grab_dpr", number(grabbed.devicePixelRatio()));
  const QRect roi = deviceRect(view.viewportTransform().mapRect(item->sceneBoundingRect()), dpr).adjusted(2, 2, -2, -2);
  const double detail = minBlockNonUniformFraction(grab, roi);
  summary.add("thumb_detail_fraction", fixed(detail, 4));
  const bool saved = saveRegionCrop(grab, roi, out + "/thumb-grab.png");

  const bool pass = saved && loaded && cacheDpr == 2.0 && grabbed.devicePixelRatio() == 2.0 && suffix == "q500"
                    && std::max(pixmap.width(), pixmap.height()) == 500 && detail >= 0.02;
  summary.add("thumbnails_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

// ---- overlays ---------------------------------------------------------------------------------------------------

bool runMaskElement(Summary& summary, const QString& out) {
  QImage page(1200, 900, QImage::Format_RGB32);
  page.fill(Qt::white);
  page.setDotsPerMeterX(11811);  // 300 dpi
  page.setDotsPerMeterY(11811);
  const QPointF squareCenter(300, 225);  // image pixels
  const double squareAngle = 30.0;       // degrees
  QImage maskImage(1200, 900, QImage::Format_RGB32);
  maskImage.fill(Qt::black);
  {
    QPainter painter(&maskImage);
    painter.translate(squareCenter);
    painter.rotate(squareAngle);
    painter.fillRect(QRectF(-100, -100, 200, 200), Qt::white);
  }
  output::PictureZoneEditor editor(page, ImagePixmapUnion(), imageproc::BinaryImage(maskImage), QTransform(),
                                   QPolygonF(QRectF(page.rect())), PageId(ImageId(QStringLiteral("page.png"))),
                                   std::make_shared<output::Settings>());
  if (!showAndExpose(editor, QSize(400, 300))) {
    return failElement(summary, "mask_", "not-exposed");
  }
  if (!waitFor([&editor] { return editor.hqPixmapIsCurrent() && editor.screenPictureMaskIsCurrent(); }, 15000)) {
    return failElement(summary, "mask_", "mask-timeout");
  }
  pump(150);

  const double dpr = editor.viewport()->devicePixelRatioF();
  const QSize expected
      = hidpi::targetRect(QRectF(0, 0, 1200, 900), editor.virtualToWidget(), editor.viewport()->size(), dpr).size();
  const double maskDpr = editor.screenPictureMask().devicePixelRatio();
  const QSize maskSize = editor.screenPictureMask().size();
  summary.add("mask_dpr", number(maskDpr));
  summary.add("mask_size", sizeText(maskSize));
  summary.add("mask_expected_size", sizeText(expected));

  const QPixmap grabbed = editor.viewport()->grab();
  const QImage grab = toRgb32(grabbed.toImage());
  summary.add("mask_grab_dpr", number(grabbed.devicePixelRatio()));

  // The rotated square's outline in device pixels, with a stroke of 6 device pixels around it.
  QTransform squareToImage;
  squareToImage.translate(squareCenter.x(), squareCenter.y());
  squareToImage.rotate(squareAngle);
  const QTransform imageToWidget = editor.imageToWidget();
  QPolygonF outline;
  for (const QPointF& corner : {QPointF(-100, -100), QPointF(100, -100), QPointF(100, 100), QPointF(-100, 100)}) {
    outline << imageToWidget.map(squareToImage.map(corner)) * dpr;
  }
  QPainterPath outlinePath;
  outlinePath.addPolygon(outline);
  outlinePath.closeSubpath();
  QPainterPathStroker stroker;
  stroker.setWidth(6.0);
  const QPainterPath band = stroker.createStroke(outlinePath);
  const double detail = minBlockNonUniformFraction(grab, band.boundingRect().toAlignedRect(),
                                                   [&band](const QPointF& p) { return band.contains(p); });
  summary.add("mask_band_detail_fraction", fixed(detail, 4));
  const QPointF centerDevice = imageToWidget.map(squareCenter) * dpr;
  const bool saved
      = saveRegionCrop(grab, QRect(centerDevice.toPoint() - QPoint(128, 128), QSize(256, 256)), out + "/mask-grab.png");

  const bool pass = saved && maskDpr == 2.0 && grabbed.devicePixelRatio() == 2.0 && maskSize == expected
                    && !expected.isEmpty() && detail >= 0.02;
  summary.add("mask_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

bool runBadgeElement(Summary& summary, const QString& out) {
  // The four lines of ThumbnailSequence::Impl::getLabelGroup that build the page badge, with the real icon pack.
  const QIcon pageThumb = IconProvider::getInstance().getIcon("left_page_thumb");
  const QSize size(23, 17);
  const QPixmap pixmap = pageThumb.pixmap(size);

  const QColor bg = qGray(QPalette().color(QPalette::WindowText).rgb()) < 128 ? QColor(Qt::white) : QColor(Qt::black);
  QGraphicsScene scene;
  scene.setBackgroundBrush(bg);
  scene.setSceneRect(0, 0, 120, 80);
  auto* item = new QGraphicsPixmapItem(pixmap);  // owned by the scene
  item->setPos(10, 10);
  scene.addItem(item);

  QGraphicsView view(&scene);
  view.setFrameShape(QFrame::NoFrame);
  view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view.setAlignment(Qt::AlignLeft | Qt::AlignTop);
  if (!showAndExpose(view, QSize(120, 80))) {
    return failElement(summary, "badge_", "not-exposed");
  }
  waitFor([&view] { return view.viewport()->size() == QSize(120, 80); }, 3000);
  pump(100);

  const double viewDpr = view.viewport()->devicePixelRatioF();
  summary.add("badge_view_dpr", number(viewDpr));
  summary.add("badge_pixmap_dpr", number(pixmap.devicePixelRatio()));
  summary.add("badge_pixmap_size", sizeText(pixmap.size()));
  summary.add("badge_bounding_rect", sizeText(item->boundingRect().size().toSize()));
  summary.add("badge_viewport_size", sizeText(view.viewport()->size()));

  const QPixmap grabbed = view.viewport()->grab();
  const QImage grab = toRgb32(grabbed.toImage());
  summary.add("badge_grab_dpr", number(grabbed.devicePixelRatio()));
  const QRect region = deviceRect(view.viewportTransform().mapRect(item->sceneBoundingRect()), viewDpr);
  summary.add("badge_device_rect",
              QString("%1,%2 %3x%4").arg(region.x()).arg(region.y()).arg(region.width()).arg(region.height()));

  QImage ref(pixmap.size(), QImage::Format_ARGB32_Premultiplied);
  ref.setDevicePixelRatio(pixmap.devicePixelRatio());
  ref.fill(bg);
  {
    QPainter painter(&ref);
    painter.drawPixmap(0, 0, pixmap);
  }
  const QImage reference = toRgb32(ref);
  if (!grab.rect().contains(region) || reference.size() != region.size()) {
    return failElement(summary, "badge_", "region-mismatch");
  }
  const Comparison cmp = compareRegion(grab, reference, region.topLeft(), region);
  summary.add("badge_grab_match_fraction", fixed(cmp.equalFraction, 4));
  summary.add("badge_grab_max_diff", QString::number(cmp.maxDiff));
  summary.add("badge_detail_fraction", fixed(minBlockNonUniformFraction(grab, region), 4));  // recorded only
  const bool saved = saveRegionCrop(grab, region, out + "/badge-grab.png");

  const bool pass = saved && pixmap.devicePixelRatio() == viewDpr && viewDpr == 2.0 && pixmap.size() == QSize(46, 34)
                    && item->boundingRect().size() == QSizeF(23, 17) && cmp.equalFraction >= 0.99 && cmp.maxDiff <= 2;
  summary.add("badge_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

struct DeskewGrab {
  QImage image;
  double dpr = 0.0;
  QSizeF viewport;
  QString reason;
};

// Shows a deskew view of the page at 600x500, waits for its HQ pixmap and grabs the viewport.
DeskewGrab grabDeskewView(const QImage& page) {
  DeskewGrab result;
  deskew::ImageView view(page, ImageViewBase::createDownscaledImage(page),
                         ImageTransformation(QRectF(page.rect()), Dpi(300, 300)));
  if (!showAndExpose(view, QSize(600, 500))) {
    result.reason = "not-exposed";
    return result;
  }
  if (!waitFor([&view] { return view.hqPixmapIsCurrent(); }, 15000)) {
    result.reason = "hq-timeout";
    return result;
  }
  pump(150);
  const QPixmap grabbed = view.viewport()->grab();
  result.dpr = grabbed.devicePixelRatio();
  result.image = toRgb32(grabbed.toImage());
  result.viewport = QSizeF(view.viewport()->size());
  return result;
}

struct Blob {
  QRect box;
  int pixels = 0;
};

bool isMagenta(const QRgb rgb) {
  return qRed(rgb) >= 215 && qGreen(rgb) <= 40 && qBlue(rgb) >= 215;
}

// 4-connected components of the pixels within 40 levels per channel of #ff00ff that have at least 64 pixels.
std::vector<Blob> findMagentaBlobs(const QImage& image) {
  std::vector<Blob> blobs;
  std::vector<char> seen(static_cast<size_t>(image.width()) * image.height(), 0);
  auto index = [&image](const int x, const int y) { return static_cast<size_t>(y) * image.width() + x; };
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      if (seen[index(x, y)] || !isMagenta(image.pixel(x, y))) {
        continue;
      }
      Blob blob;
      std::vector<QPoint> stack;
      stack.push_back(QPoint(x, y));
      seen[index(x, y)] = 1;
      while (!stack.empty()) {
        const QPoint p = stack.back();
        stack.pop_back();
        blob.box = blob.box.isNull() ? QRect(p, QSize(1, 1)) : blob.box.united(QRect(p, QSize(1, 1)));
        ++blob.pixels;
        const QPoint neighbours[4]
            = {QPoint(p.x() + 1, p.y()), QPoint(p.x() - 1, p.y()), QPoint(p.x(), p.y() + 1), QPoint(p.x(), p.y() - 1)};
        for (const QPoint& n : neighbours) {
          if (n.x() < 0 || n.y() < 0 || n.x() >= image.width() || n.y() >= image.height()) {
            continue;
          }
          if (!seen[index(n.x(), n.y())] && isMagenta(image.pixel(n.x(), n.y()))) {
            seen[index(n.x(), n.y())] = 1;
            stack.push_back(n);
          }
        }
      }
      if (blob.pixels >= 64) {
        blobs.push_back(blob);
      }
    }
  }
  return blobs;
}

bool runHandleElement(Summary& summary, const QString& out) {
  const QImage page = test_support::generatePage({test_support::PageKind::FLAT, 300, 1, 0.0}).image;
  if (page.isNull()) {
    return failElement(summary, "handle_", "no-page");
  }

  // Placement run: the handles are opaque magenta squares, so they cannot be confused with the blue grid and arcs.
  const QString iconDir = out + "/icons";
  QDir().mkpath(iconDir);
  const QString square = iconDir + "/magenta.svg";
  const QString sheet = iconDir + "/icons.xml";
  const QByteArray svg
      = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 16 16\" width=\"16\" height=\"16\">"
        "<rect x=\"0\" y=\"0\" width=\"16\" height=\"16\" fill=\"#ff00ff\"/></svg>";
  const QByteArray xml
      = "<icons><icon name=\"aqua-sphere\"><file>" + square.toHtmlEscaped().toUtf8() + "</file></icon></icons>";
  if (!writeBytes(square, svg) || !writeBytes(sheet, xml)) {
    return failElement(summary, "handle_", "cannot-write-icons");
  }
  DeskewGrab placement;
  {
    const DefaultIconPackRestorer restorer;
    IconProvider::getInstance().setIconPack(std::make_unique<StyledIconPack>(sheet));
    placement = grabDeskewView(page);
  }
  if (!placement.reason.isEmpty()) {
    return failElement(summary, "handle_", "placement-" + placement.reason);
  }

  std::vector<Blob> blobs = findMagentaBlobs(placement.image);
  summary.add("handle_blob_count", QString::number(blobs.size()));
  std::sort(blobs.begin(), blobs.end(), [](const Blob& a, const Blob& b) { return a.pixels > b.pixels; });
  if (blobs.size() < 4) {
    return failElement(summary, "handle_", "fewer-than-four-blobs");
  }
  blobs.resize(4);
  const double dpr = placement.dpr;
  int maxSide = 0;
  for (const Blob& blob : blobs) {
    maxSide = std::max({maxSide, blob.box.width(), blob.box.height()});
  }
  summary.add("handle_blob_max_side", QString::number(maxSide));
  auto centerX = [dpr](const Blob& b) { return 0.5 * (b.box.left() + b.box.right() + 1) / dpr; };
  auto centerY = [dpr](const Blob& b) { return 0.5 * (b.box.top() + b.box.bottom() + 1) / dpr; };
  std::vector<Blob> byX = blobs;
  std::sort(byX.begin(), byX.end(), [&](const Blob& a, const Blob& b) { return centerX(a) < centerX(b); });
  std::vector<Blob> byY = blobs;
  std::sort(byY.begin(), byY.end(), [&](const Blob& a, const Blob& b) { return centerY(a) < centerY(b); });
  const double horizontalOffset
      = std::abs(0.5 * (centerX(byX.front()) + centerX(byX.back())) - 0.5 * placement.viewport.width());
  const double verticalOffset
      = std::abs(0.5 * (centerY(byY.front()) + centerY(byY.back())) - 0.5 * placement.viewport.height());
  summary.add("handle_horizontal_offset", fixed(horizontalOffset, 3));
  summary.add("handle_vertical_offset", fixed(verticalOffset, 3));
  summary.add("handle_viewport_size",
              QString("%1x%2").arg(placement.viewport.width()).arg(placement.viewport.height()));

  // Sharpness run with the real icon, same layout.
  const QPixmap handlePixmap = IconProvider::getInstance().getIcon("aqua-sphere").pixmap(16, 16);
  summary.add("handle_pixmap_dpr", number(handlePixmap.devicePixelRatio()));
  summary.add("handle_logical_size", sizeText(hidpi::logicalSize(handlePixmap).toSize()));
  const DeskewGrab sharp = grabDeskewView(page);
  if (!sharp.reason.isEmpty()) {
    return failElement(summary, "handle_", "sharpness-" + sharp.reason);
  }
  QRect united;
  for (const Blob& blob : blobs) {
    united = united.isNull() ? blob.box : united.united(blob.box);
  }
  const double detail = minBlockNonUniformFraction(sharp.image, united, [&blobs](const QPointF& p) {
    for (const Blob& blob : blobs) {
      if (blob.box.contains(p.toPoint())) {
        return true;
      }
    }
    return false;
  });
  summary.add("handle_detail_fraction", fixed(detail, 4));
  const Blob& right = byX.back();
  const bool saved = saveRegionCrop(sharp.image, right.box.adjusted(-8, -8, 8, 8), out + "/handles-grab.png");

  const bool pass = saved && dpr == 2.0 && handlePixmap.devicePixelRatio() == 2.0
                    && hidpi::logicalSize(handlePixmap).toSize() == QSize(16, 16) && maxSide >= 30 && maxSide <= 34
                    && horizontalOffset <= 2.0 && verticalOffset <= 2.0 && detail >= 0.02;
  summary.add("handle_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

bool runStageElement(Summary& summary, const QString& out) {
  const auto provider = std::make_shared<EmptySelectionProvider>();
  const auto stages
      = std::make_shared<StageSequence>(std::make_shared<ProjectPages>(), PageSelectionAccessor(provider));
  StageListView view(nullptr);
  view.setStages(stages);
  if (!showAndExpose(view, QSize(300, 400))) {
    return failElement(summary, "stage_", "not-exposed");
  }
  view.selectRow(0);
  view.setBatchProcessingPossible(true);
  view.setBatchProcessingInProgress(true);
  pump(600);

  if (view.batchAnimationFrames().empty()) {
    return failElement(summary, "stage_", "no-frames");
  }
  const QPixmap& frame = view.batchAnimationFrames().front();
  summary.add("stage_frame_dpr", number(frame.devicePixelRatio()));
  summary.add("stage_frame_size", sizeText(frame.size()));
  const QSize logicalFrame = hidpi::logicalSize(frame).toSize();
  summary.add("stage_frame_logical_size", sizeText(logicalFrame));

  const double dpr = view.viewport()->devicePixelRatioF();
  const QRect cell = view.visualRect(view.model()->index(0, 1));
  summary.add("stage_cell_rect",
              QString("%1,%2 %3x%4").arg(cell.x()).arg(cell.y()).arg(cell.width()).arg(cell.height()));
  const QRect cellDevice = deviceRect(cell, dpr);
  // The outermost logical pixel of the cell is left out (grid lines).
  const QRect roi = cellDevice.adjusted(static_cast<int>(dpr), static_cast<int>(dpr), -static_cast<int>(dpr),
                                        -static_cast<int>(dpr));

  // One frame shows only part of the ring (a comet whose tail fades into the background), so the ink of 12 grabs, one
  // per animation step, is united: every one of the 8 positions of the ring carries the head colour in some frame.
  QRect box;
  std::vector<QImage> grabs;
  for (int i = 0; i < 12; ++i) {
    pump(190);
    const QPixmap grabbed = view.viewport()->grab();
    grabs.push_back(toRgb32(grabbed.toImage()));
    const QImage& grab = grabs.back();
    if (cell.isEmpty() || !grab.rect().contains(cellDevice)) {
      return failElement(summary, "stage_", "cell-out-of-grab");
    }
    const QRect sampleBox = inkBox(grab, roi, grab.pixel(roi.topLeft()), 30);
    if (!sampleBox.isNull()) {
      box = box.isNull() ? sampleBox : box.united(sampleBox);
    }
  }
  if (box.isNull()) {
    return failElement(summary, "stage_", "no-ink");
  }
  summary.add("stage_ink_box", QString("%1,%2 %3x%4").arg(box.x()).arg(box.y()).arg(box.width()).arg(box.height()));
  const QPointF inkCenter(0.5 * (box.left() + box.right() + 1) / dpr, 0.5 * (box.top() + box.bottom() + 1) / dpr);
  const QPointF cellCenter(cell.x() + 0.5 * cell.width(), cell.y() + 0.5 * cell.height());
  const double offset = std::hypot(inkCenter.x() - cellCenter.x(), inkCenter.y() - cellCenter.y());
  summary.add("stage_ink_center_offset", fixed(offset, 3));
  // The best of the grabs: a frame shows a part of the ring only, so the box is mostly background in any one of them.
  double detail = 0.0;
  size_t bestGrab = 0;
  for (size_t i = 0; i < grabs.size(); ++i) {
    const double sampleDetail = minBlockNonUniformFraction(grabs[i], box);
    if (sampleDetail > detail) {
      detail = sampleDetail;
      bestGrab = i;
    }
  }
  summary.add("stage_detail_fraction", fixed(detail, 4));
  const bool saved = saveRegionCrop(grabs[bestGrab], box.adjusted(-8, -8, 8, 8), out + "/stage-grab.png");

  const bool pass
      = saved && frame.devicePixelRatio() == 2.0 && frame.size() == logicalFrame * 2 && offset <= 1.5 && detail >= 0.02;
  summary.add("stage_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

bool runDecorationElement(Summary& summary, const QString& out) {
  // The RelinkingModel pattern: a file icon pixmap as model data in an item view.
  const QPixmap icon = IconProvider::getInstance().getIcon("file").pixmap(16, 16);
  QStandardItemModel model;
  auto* item = new QStandardItem(QStringLiteral("file"));  // owned by the model
  item->setData(icon, Qt::DecorationRole);
  model.appendRow(item);
  QListView view;
  view.setModel(&model);
  if (!showAndExpose(view, QSize(200, 60))) {
    return failElement(summary, "decoration_", "not-exposed");
  }
  pump(100);

  const double dpr = view.viewport()->devicePixelRatioF();
  const QPixmap grabbed = view.viewport()->grab();
  const QImage grab = toRgb32(grabbed.toImage());
  const QRect itemRect = view.visualRect(model.index(0, 0));
  const QRect itemDevice = deviceRect(itemRect, dpr);
  if (itemRect.isEmpty() || !grab.rect().contains(itemDevice)) {
    return failElement(summary, "decoration_", "item-out-of-grab");
  }
  // The ink of the first run of adjacent columns: the icon, not the text that follows after a gap.
  const QRect left(itemDevice.topLeft(), QSize(static_cast<int>(24 * dpr), itemDevice.height()));
  const QRgb background = grab.pixel(itemDevice.topLeft());
  int runStart = -1;
  int runEnd = -1;
  for (int x = left.left(); x <= left.right(); ++x) {
    if (inkBox(grab, QRect(x, left.top(), 1, left.height()), background, 30).isNull()) {
      if (runStart >= 0) {
        break;
      }
    } else {
      runStart = runStart < 0 ? x : runStart;
      runEnd = x;
    }
  }
  const QRect box
      = runStart < 0 ? QRect()
                     : inkBox(grab, QRect(runStart, left.top(), runEnd - runStart + 1, left.height()), background, 30);
  summary.add("decoration_pixmap_dpr", number(icon.devicePixelRatio()));
  if (box.isNull()) {
    return failElement(summary, "decoration_", "no-ink");
  }
  summary.add("decoration_drawn_size", sizeText(box.size()));
  // The opaque part of the pixmap itself (device pixels), recorded to compare with the drawn size.
  const QImage iconImage = icon.toImage().convertToFormat(QImage::Format_ARGB32);
  QRect iconInk;
  for (int y = 0; y < iconImage.height(); ++y) {
    for (int x = 0; x < iconImage.width(); ++x) {
      if (qAlpha(iconImage.pixel(x, y)) > 128) {
        iconInk = iconInk.isNull() ? QRect(x, y, 1, 1) : iconInk.united(QRect(x, y, 1, 1));
      }
    }
  }
  summary.add("decoration_pixmap_size", sizeText(icon.size()));
  summary.add("decoration_pixmap_ink_size", sizeText(iconInk.size()));
  const double detail = minBlockNonUniformFraction(grab, box);
  summary.add("decoration_detail_fraction", fixed(detail, 4));
  const bool saved = saveRegionCrop(grab, box.adjusted(-8, -8, 8, 8), out + "/decoration-grab.png");

  const bool pass = saved && icon.devicePixelRatio() == 2.0 && box.width() <= 34 && box.height() <= 34
                    && box.width() >= 20 && box.height() >= 20 && detail >= 0.02;
  summary.add("decoration_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

bool runOverlaysSection(Summary& summary, const QString& out) {
  bool pass = runMaskElement(summary, out);
  pass = runBadgeElement(summary, out) && pass;
  pass = runHandleElement(summary, out) && pass;
  pass = runStageElement(summary, out) && pass;
  pass = runDecorationElement(summary, out) && pass;
  summary.add("overlays_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

// ---- picker -----------------------------------------------------------------------------------------------------

bool runPickerSection(Summary& summary) {
  const QColor patchColor(220, 120, 30);
  QImage page(1200, 900, QImage::Format_RGB32);
  page.fill(QColor(40, 60, 200));
  {
    QPainter painter(&page);
    painter.fillRect(QRect(360, 270, 480, 360), patchColor);
  }
  page.setDotsPerMeterX(11811);  // 300 dpi
  page.setDotsPerMeterY(11811);

  BasicImageView view(page);
  if (!showAndExpose(view, QSize(400, 300))) {
    return failElement(summary, "picker_", "not-exposed");
  }
  if (!waitFor([&view] { return view.hqPixmapIsCurrent(); }, 15000)) {
    return failElement(summary, "picker_", "hq-timeout");
  }
  pump(150);

  QRect rect(0, 0, 15, 15);
  rect.moveCenter(view.rect().center());
  const QPixmap grabbed = view.grab(rect);
  summary.add("picker_grab_size", sizeText(grabbed.size()));
  summary.add("picker_grab_dpr", number(grabbed.devicePixelRatio()));
  const QColor picked = output::ColorPickupInteraction::medianColor(grabbed.toImage());
  summary.add("picker_color", picked.isValid() ? picked.name() : QString("invalid"));
  summary.add("picker_expected", patchColor.name());

  const bool pass = grabbed.size() == QSize(30, 30) && grabbed.devicePixelRatio() == 2.0 && picked.isValid()
                    && picked.rgb() == patchColor.rgb();
  summary.add("picker_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

}  // namespace

int main(int argc, char* argv[]) {
  QString outArg;
  QString section = "page";
  bool badArguments = false;
  int positional = 0;
  for (int i = 1; i < argc; ++i) {
    const QString arg = QString::fromLocal8Bit(argv[i]);
    if (arg.startsWith("-Apple")) {
      ++i;  // AppKit's argument domain, e.g. -ApplePersistenceIgnoreState YES
    } else if (arg == "--section" && i + 1 < argc) {
      section = QString::fromLocal8Bit(argv[++i]);
    } else if (positional == 0 && !arg.startsWith("-")) {
      outArg = arg;
      ++positional;
    } else {
      badArguments = true;
    }
  }
  const QStringList knownSections = {"page", "thumbnails", "overlays", "picker", "all"};
  if (badArguments || positional != 1 || !knownSections.contains(section)) {
    std::fprintf(stderr, "%s", kUsage);
    return 2;
  }

  QString out;
  QString guardError;
  if (!harness::outputDirIsStrictlyInside(outArg, QString::fromUtf8(RETINA_CHECK_ROOT), &out, &guardError)) {
    std::fprintf(stderr, "retina_check: %s\n", guardError.toLocal8Bit().constData());
    return 2;
  }
  if (!QDir().mkpath(out + "/tmp") || !QDir().mkpath(out + "/settings")) {
    std::fprintf(stderr, "retina_check: cannot create %s\n", out.toLocal8Bit().constData());
    return 2;
  }

  // Everything below is set before the application object exists, so no setting is read from the user's account
  // and the measurement sees bare Cocoa behaviour.
  qunsetenv("QT_QPA_PLATFORM");
  qunsetenv("QT_SCALE_FACTOR");
  qunsetenv("QT_SCREEN_SCALE_FACTORS");
  qunsetenv("QT_ENABLE_HIGHDPI_SCALING");
  qunsetenv("QT_SCALE_FACTOR_ROUNDING_POLICY");
  qunsetenv("QT_USE_PHYSICAL_DPI");
  qputenv("LC_ALL", "en_US.UTF-8");
  qputenv("TMPDIR", (out + "/tmp").toUtf8());

  QCoreApplication::setOrganizationName("scantailor-tests");
  QCoreApplication::setApplicationName("retina_check");
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, out + "/settings");

  Application app(argc, argv);

  const QString settingsFile = QSettings().fileName();
  if (!settingsFile.startsWith(out + "/settings/")) {
    std::fprintf(stderr, "retina_check: settings file %s is not under %s/settings\n",
                 settingsFile.toLocal8Bit().constData(), out.toLocal8Bit().constData());
    return 2;
  }
  if (QGuiApplication::platformName() != "cocoa") {
    std::fprintf(stderr, "retina_check: platform is %s, not cocoa\n",
                 QGuiApplication::platformName().toLocal8Bit().constData());
    return 3;
  }

  {
    std::unique_ptr<ColorScheme> scheme
        = ColorSchemeFactory().create(ApplicationSettings::getInstance().getColorScheme());
    ColorSchemeManager::instance().setColorScheme(*scheme);
  }
  IconProvider::getInstance().setIconPack(StyledIconPack::createDefault());

  Summary summary;
  summary.add("platform", QGuiApplication::platformName());
  summary.add("qt_runtime", QString::fromLatin1(qVersion()));
  summary.add("section", section);

  bool pass = true;
  if (section == "page" || section == "all") {
    pass = runPageSection(summary, out) && pass;
  }
  if (section == "thumbnails" || section == "all") {
    pass = runThumbnailsSection(summary, out) && pass;
  }
  if (section == "overlays" || section == "all") {
    pass = runOverlaysSection(summary, out) && pass;
  }
  if (section == "picker" || section == "all") {
    pass = runPickerSection(summary) && pass;
  }
  summary.add("verdict", pass ? "PASS" : "FAIL");
  if (!summary.save(out + "/summary.txt")) {
    std::fprintf(stderr, "retina_check: cannot write %s/summary.txt\n", out.toLocal8Bit().constData());
    return 2;
  }
  return pass ? 0 : 1;
}
