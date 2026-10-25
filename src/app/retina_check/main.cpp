// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
//
// Live Retina check for Phase 4 (D-02): shows a page view on the default Cocoa platform, records the device pixel
// ratio of screen, window and view, compares the page view with a device-resolution reference of the same synthetic
// page at fit, 1:1 and 2:1 zoom, and measures whether the 0.5 mm smoothing switch counts physical millimetres.
// usage: retina_check <output-dir> [--section page]
// The output directory must lie below the checkout's build/ directory. QSettings and temporary files are kept
// inside it, so a run never touches the user's real settings. The tool runs on the platform Qt picks by default
// (cocoa) and exits with 3 on any other platform, so an offscreen or scaled run cannot pass for a Retina run.

#include <BasicImageView.h>
#include <HiDpi.h>
#include <PageGenerator.h>
#include <Transform.h>
#include <core/Application.h>
#include <core/ApplicationSettings.h>
#include <core/ColorSchemeFactory.h>
#include <core/ColorSchemeManager.h>
#include <core/IconProvider.h>
#include <core/StyledIconPack.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QScreen>
#include <QSettings>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QTransform>
#include <QWindow>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <memory>

namespace {

const char* const kUsage = "usage: retina_check <output-dir> [--section page]\n";

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

int runPageSection(Summary& summary, const QString& outDir) {
  const QImage page = test_support::generatePage({test_support::PageKind::FLAT, 300, 1, 0.0}).image;
  if (page.isNull()) {
    summary.add("page_verdict", "FAIL");
    summary.add("page_reason", "no-page");
    summary.add("verdict", "FAIL");
    return 1;
  }

  BasicImageView view(page);
  view.resize(1280, 900);
  view.show();
  view.raise();
  if (!waitFor([&view] { return view.windowHandle() && view.windowHandle()->isExposed(); }, 5000)) {
    summary.add("page_verdict", "FAIL");
    summary.add("page_reason", "not-exposed");
    summary.add("verdict", "FAIL");
    return 1;
  }

  const QScreen* screen = view.screen();
  const double windowDpr = view.windowHandle()->devicePixelRatio();
  const double screenMmPerLogicalPx
      = screen->geometry().width() > 0 ? screen->physicalSize().width() / screen->geometry().width() : 0.0;

  summary.add("platform", QGuiApplication::platformName());
  summary.add("qt_runtime", QString::fromLatin1(qVersion()));
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
    summary.add("verdict", "FAIL");
    return 1;
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
  summary.add("verdict", pass ? "PASS" : "FAIL");
  return pass ? 0 : 1;
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
  if (badArguments || positional != 1 || section != "page") {
    std::fprintf(stderr, "%s", kUsage);
    return 2;
  }

  const QString root = QDir::cleanPath(QString::fromUtf8(RETINA_CHECK_ROOT));
  const QString out = QDir::cleanPath(QDir::current().absoluteFilePath(outArg));
  if (out != root && !out.startsWith(root + "/")) {
    std::fprintf(stderr, "retina_check: %s is not under %s\n", out.toLocal8Bit().constData(),
                 root.toLocal8Bit().constData());
    return 2;
  }
  if (!QDir().mkpath(out + "/tmp") || !QDir().mkpath(out + "/settings")) {
    std::fprintf(stderr, "retina_check: cannot create %s\n", out.toLocal8Bit().constData());
    return 2;
  }

  // Everything below is set before the application object exists, so no setting is read from the user's account
  // and the measurement sees bare Cocoa behaviour (D-00c).
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
  const int result = runPageSection(summary, out);
  if (!summary.save(out + "/summary.txt")) {
    std::fprintf(stderr, "retina_check: cannot write %s/summary.txt\n", out.toLocal8Bit().constData());
    return 2;
  }
  return result;
}
