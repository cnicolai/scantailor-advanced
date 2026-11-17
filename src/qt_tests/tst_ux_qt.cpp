// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_ux_qt.h"

#include <CollapsibleGroupBox.h>
#include <Dpi.h>
#include <ErrorWidget.h>
#include <FilterOptionsWidget.h>
#include <FilterResult.h>
#include <FilterUiInterface.h>
#include <ForgedImages.h>
#include <ImageId.h>
#include <ImageInfo.h>
#include <ImageMetadata.h>
#include <LoadFileTask.h>
#include <PageId.h>
#include <PageInfo.h>
#include <ProcessMemory.h>
#include <ProjectPages.h>
#include <ThumbnailLoadResult.h>
#include <ThumbnailPixmapCache.h>
#include <filters/fix_orientation/Task.h>

#include "ProjectFilesDialog.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QBrush>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolButton>
#include <QWidget>
#include <QtTest/QtTest>
#include <memory>
#include <vector>

void TestUxQt::testButtonClickEmitsSignal() {
  QPushButton button("Click me");
  QSignalSpy spy(&button, &QPushButton::clicked);
  QVERIFY(spy.isValid());

  QTest::mouseClick(&button, Qt::LeftButton);
  QCOMPARE(spy.count(), 1);

  QTest::mouseClick(&button, Qt::LeftButton);
  QCOMPARE(spy.count(), 2);
}

void TestUxQt::testCheckBoxToggleChangesState() {
  QCheckBox box("Option");
  QVERIFY(!box.isChecked());

  box.setChecked(true);
  QVERIFY(box.isChecked());

  box.setChecked(false);
  QVERIFY(!box.isChecked());

  QSignalSpy spy(&box, &QCheckBox::toggled);
  box.setChecked(true);
  QCOMPARE(spy.count(), 1);
  QVERIFY(box.isChecked());
}

void TestUxQt::testLineEditAcceptsInput() {
  QLineEdit edit;
  QCOMPARE(edit.text(), QString());

  QTest::keyClicks(&edit, "Hello UX");
  QCOMPARE(edit.text(), QString("Hello UX"));

  edit.clear();
  QTest::keyClicks(&edit, "123");
  QCOMPARE(edit.text(), QString("123"));
}

void TestUxQt::testCollapsibleGroupBoxStateAndSignal() {
  CollapsibleGroupBox group("Options");
  QSignalSpy spy(&group, &CollapsibleGroupBox::collapsedStateChanged);
  QVERIFY(spy.isValid());

  QVERIFY(!group.isCollapsed());

  group.setCollapsed(true);
  QVERIFY(group.isCollapsed());
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toBool(), true);

  group.setCollapsed(false);
  QVERIFY(!group.isCollapsed());
  QCOMPARE(spy.count(), 2);
  QCOMPARE(spy.at(1).at(0).toBool(), false);
}

void TestUxQt::testErrorWidgetCreation() {
  ErrorWidget widget("Error de prueba", Qt::PlainText);
  QVERIFY(widget.findChildren<QWidget*>().size() >= 1);
}

namespace {

// Records what a filter result puts on screen, in place of the main window.
class RecordingUi : public FilterUiInterface {
 public:
  void setOptionsWidget(FilterOptionsWidget* widget, const Ownership ownership) override {
    if (ownership == TRANSFER_OWNERSHIP) {
      m_optionsWidget.reset(widget);
    }
  }

  void setImageWidget(QWidget* widget, const Ownership ownership, DebugImages*, bool) override {
    m_imageWidget = widget;
    if (ownership == TRANSFER_OWNERSHIP) {
      m_ownedImageWidget.reset(widget);
    }
  }

  void invalidateThumbnail(const PageId&) override {}

  void invalidateAllThumbnails() override {}

  std::shared_ptr<AbstractCommand<void>> relinkingDialogRequester() override {
    class NoOp : public AbstractCommand<void> {
     public:
      void operator()() override {}
    };
    return std::make_shared<NoOp>();
  }

  QWidget* imageWidget() const { return m_imageWidget; }

 private:
  QWidget* m_imageWidget = nullptr;
  std::unique_ptr<QWidget> m_ownedImageWidget;
  std::unique_ptr<FilterOptionsWidget> m_optionsWidget;
};

}  // namespace

void TestUxQt::testOversizedImageShowsTooLargeInPageView_data() {
  QTest::addColumn<QByteArray>("bytes");
  QTest::addColumn<QString>("fileName");
  QTest::addColumn<bool>("tooLarge");
  QTest::addColumn<QString>("sizeText");

  // A header that claims 100000 x 100000 px in a file of a few hundred bytes.
  QTest::newRow("tiff") << test_support::forgedTiff({{100000, 100000, 1, 1, false}}) << QString("oversized.tif") << true
                        << QString::fromUtf8("100000 \u00d7 100000 px");
  QTest::newRow("png") << test_support::forgedPng(100000, 100000) << QString("oversized.png") << true
                       << QString::fromUtf8("100000 \u00d7 100000 px");
  // Above libpng's default limit of 1000000 px per side: the metadata loader must still report the size.
  QTest::newRow("png-wide") << test_support::forgedPng(2000000, 1) << QString("wide.png") << true
                            << QString::fromUtf8("2000000 \u00d7 1 px");
  QTest::newRow("jpeg") << test_support::forgedJpeg(65500, 65500, true) << QString("oversized.jpg") << true
                        << QString::fromUtf8("65500 \u00d7 65500 px");
  QTest::newRow("garbage") << QByteArray("not an image") << QString("garbage.tif") << false << QString();
}

void TestUxQt::testOversizedImageShowsTooLargeInPageView() {
  QFETCH(QByteArray, bytes);
  QFETCH(QString, fileName);
  QFETCH(bool, tooLarge);
  QFETCH(QString, sizeText);

  QVERIFY(!bytes.isEmpty());
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QVERIFY(dir.path().startsWith(QString::fromUtf8(TEST_TMP_ROOT)));
  const QString filePath = dir.filePath(fileName);
  {
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(bytes), qint64(bytes.size()));
  }

  const ImageId imageId(filePath);
  const PageId pageId(imageId, PageId::SINGLE_PAGE);
  const ImageMetadata metadata(QSize(100000, 100000), Dpi(300, 300));
  const std::vector<ImageInfo> images{ImageInfo(imageId, metadata, 1, false, false)};
  const auto pages = std::make_shared<ProjectPages>(images, Qt::LeftToRight);
  const auto thumbnails = std::make_shared<ThumbnailPixmapCache>(dir.filePath("thumbs"), QSize(100, 100), 5, 3);
  const auto nextTask = std::make_shared<fix_orientation::Task>(pageId, nullptr, nullptr, nullptr, nullptr, false);

  LoadFileTask task(BackgroundTask::INTERACTIVE, PageInfo(pageId, metadata, 1, false, false), thumbnails, pages,
                    nextTask);
  QElapsedTimer timer;
  timer.start();
  const FilterResultPtr result = task();
  QVERIFY2(timer.elapsed() < 2000, "refusing the file must not allocate the claimed size");
  QVERIFY(result);

  RecordingUi ui;
  result->updateUI(&ui);
  QVERIFY(ui.imageWidget());
  const QLabel* label = ui.imageWidget()->findChild<QLabel*>("textLabel");
  QVERIFY(label);
  const QString text = label->text();

  if (tooLarge) {
    QVERIFY2(text.contains("too large to load"), qPrintable(text));
    QVERIFY2(text.contains(sizeText), qPrintable(text));
    QVERIFY2(text.contains("65535 px per side"), qPrintable(text));
    QVERIFY2(!text.contains("could not be loaded"), qPrintable(text));
  } else {
    QVERIFY2(text.contains("could not be loaded"), qPrintable(text));
    QVERIFY2(!text.contains("too large"), qPrintable(text));
  }
}

namespace {

// Fills a temporary folder the way a user's scan folder would look for the project file dialog.
void writeBytes(const QString& path, const QByteArray& bytes) {
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));
  QCOMPARE(file.write(bytes), qint64(bytes.size()));
}

void writeNormalPng(const QString& path) {
  QImage image(64, 64, QImage::Format_RGB32);
  image.fill(Qt::white);
  image.setDotsPerMeterX(11811);  // 300 dpi
  image.setDotsPerMeterY(11811);
  QVERIFY(image.save(path, "PNG"));
}

// Closes the warning box of the dialog as soon as it is on screen and keeps its text.
class BoxCloser {
 public:
  BoxCloser() {
    m_timer.setInterval(50);
    QObject::connect(&m_timer, &QTimer::timeout, [this]() {
      if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
        m_texts.push_back(box->text());
        box->accept();
      }
    });
    m_timer.start();
  }

  const QStringList& texts() const { return m_texts; }

 private:
  QTimer m_timer;
  QStringList m_texts;
};

}  // namespace

void TestUxQt::testProjectFilesDialogMarksOversizedFile() {
  const QString tmpRoot = QString::fromUtf8(TEST_TMP_ROOT);

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QVERIFY(dir.path().startsWith(tmpRoot));
  writeBytes(dir.filePath("forged.tif"), test_support::forgedTiff({{100000, 100000, 1, 1, false}}));
  writeBytes(dir.filePath("two-pages.tif"),
             test_support::forgedTiff({{64, 64, 8, 1, true}, {100000, 100000, 1, 1, false}}));
  writeNormalPng(dir.filePath("normal.png"));

  {
    ProjectFilesDialog dialog;
    QVERIFY(QMetaObject::invokeMethod(&dialog, "inpDirEdited", Qt::DirectConnection, Q_ARG(QString, dir.path())));
    auto* selectAll = dialog.findChild<QPushButton*>("offProjectSelectAllBtn");
    auto* add = dialog.findChild<QToolButton*>("addToProjectBtn");
    auto* buttons = dialog.findChild<QDialogButtonBox*>("buttonBox");
    auto* list = dialog.findChild<QListView*>("inProjectList");
    QVERIFY(selectAll && add && buttons && list);
    QTest::mouseClick(selectAll, Qt::LeftButton);
    QTest::mouseClick(add, Qt::LeftButton);
    QCOMPARE(list->model()->rowCount(), 3);

    BoxCloser closer;
    QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QTRY_VERIFY(!closer.texts().isEmpty());
    QCOMPARE(closer.texts().size(), 1);
    QVERIFY2(closer.texts().first().contains("Some of the files failed to load."), qPrintable(closer.texts().first()));
    QVERIFY2(closer.texts().first().contains("65535 px per side"), qPrintable(closer.texts().first()));

    const QString forged = QString::fromUtf8("forged.tif (too large: 100000 \u00d7 100000 px)");
    const QString twoPages = QString::fromUtf8("two-pages.tif (page 2 too large: 100000 \u00d7 100000 px)");
    QMap<QString, QColor> colours;
    for (int row = 0; row < list->model()->rowCount(); ++row) {
      const QModelIndex index = list->model()->index(row, 0);
      colours[index.data(Qt::DisplayRole).toString()] = index.data(Qt::ForegroundRole).value<QBrush>().color();
    }
    QVERIFY2(colours.contains(forged), qPrintable(QStringList(colours.keys()).join(" | ")));
    QVERIFY2(colours.contains(twoPages), qPrintable(QStringList(colours.keys()).join(" | ")));
    QVERIFY2(colours.contains("normal.png"), qPrintable(QStringList(colours.keys()).join(" | ")));
    QCOMPARE(colours.value(forged), QColor(0xff, 0x00, 0x00));
    QCOMPARE(colours.value(twoPages), QColor(0xff, 0x00, 0x00));
    QCOMPARE(colours.value("normal.png"), QColor(0x00, 0xff, 0x00));
    QVERIFY(dialog.result() != QDialog::Accepted);
  }

  // Control: a folder of normal files is accepted without any box.
  QTemporaryDir cleanDir;
  QVERIFY(cleanDir.isValid());
  QVERIFY(cleanDir.path().startsWith(tmpRoot));
  writeNormalPng(cleanDir.filePath("normal.png"));

  ProjectFilesDialog dialog2;
  QVERIFY(QMetaObject::invokeMethod(&dialog2, "inpDirEdited", Qt::DirectConnection, Q_ARG(QString, cleanDir.path())));
  auto* selectAll2 = dialog2.findChild<QPushButton*>("offProjectSelectAllBtn");
  auto* add2 = dialog2.findChild<QToolButton*>("addToProjectBtn");
  auto* buttons2 = dialog2.findChild<QDialogButtonBox*>("buttonBox");
  QVERIFY(selectAll2 && add2 && buttons2);
  QTest::mouseClick(selectAll2, Qt::LeftButton);
  QTest::mouseClick(add2, Qt::LeftButton);

  BoxCloser closer2;
  QTest::mouseClick(buttons2->button(QDialogButtonBox::Ok), Qt::LeftButton);
  QTRY_COMPARE(dialog2.result(), int(QDialog::Accepted));
  QVERIFY(closer2.texts().isEmpty());
  QCOMPARE(dialog2.inProjectFiles().size(), size_t(1));
}

namespace {

class RecordingThumbnailHandler : public ThumbnailPixmapCache::CompletionHandler {
 public:
  void operator()(const ThumbnailLoadResult& result) override {
    m_status = result.status();
    m_fired = true;
  }

  bool fired() const { return m_fired; }

  ThumbnailLoadResult::Status status() const { return m_status; }

 private:
  ThumbnailLoadResult::Status m_status = ThumbnailLoadResult::REQUEST_EXPIRED;
  bool m_fired = false;
};

// Requests the thumbnail of one file through the cache (a background thread loads the source image) and waits for
// the completion handler. The peak footprint growth is measured around the whole request.
void requestThumbnail(const QString& filePath,
                      const QString& thumbDir,
                      ThumbnailLoadResult::Status* status,
                      qint64* peakGrowthBytes) {
  QVERIFY(QDir().mkpath(thumbDir));
  ThumbnailPixmapCache cache(thumbDir, QSize(100, 100), 5, 3);
  const auto handler = std::make_shared<RecordingThumbnailHandler>();
  QPixmap pixmap;

  const qint64 peakBefore = test_support::peakPhysFootprintBytes();
  const ThumbnailPixmapCache::Status requested
      = cache.loadRequest(ImageId(filePath), pixmap, std::weak_ptr<ThumbnailPixmapCache::CompletionHandler>(handler));
  if (requested == ThumbnailPixmapCache::QUEUED) {
    QTRY_VERIFY_WITH_TIMEOUT(handler->fired(), 20000);
    *status = handler->status();
  } else {
    *status
        = (requested == ThumbnailPixmapCache::LOADED) ? ThumbnailLoadResult::LOADED : ThumbnailLoadResult::LOAD_FAILED;
  }
  *peakGrowthBytes = test_support::peakPhysFootprintBytes() - peakBefore;
}

}  // namespace

void TestUxQt::testOversizedThumbnailFails() {
  const qint64 kBoundBytes = qint64(64) * 1024 * 1024;

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QVERIFY(dir.path().startsWith(QString::fromUtf8(TEST_TMP_ROOT)));
  writeBytes(dir.filePath("forged.tif"), test_support::forgedTiff({{100000, 100000, 1, 1, false}}));
  writeNormalPng(dir.filePath("normal.png"));

  ThumbnailLoadResult::Status status = ThumbnailLoadResult::REQUEST_EXPIRED;
  qint64 growth = 0;

  const QString forgedThumbs = dir.filePath("thumbs-forged");
  requestThumbnail(dir.filePath("forged.tif"), forgedThumbs, &status, &growth);
  QCOMPARE(int(status), int(ThumbnailLoadResult::LOAD_FAILED));
  QVERIFY2(QDir(forgedThumbs).entryList(QDir::Files | QDir::NoDotAndDotDot).isEmpty(),
           "a refused source image must leave no thumbnail file");
  QVERIFY2(growth < kBoundBytes, qPrintable(QString("peak footprint grew by %1 bytes").arg(growth)));

  // Control: a normal file gives a thumbnail and a file.
  const QString normalThumbs = dir.filePath("thumbs-normal");
  requestThumbnail(dir.filePath("normal.png"), normalThumbs, &status, &growth);
  QCOMPARE(int(status), int(ThumbnailLoadResult::LOADED));
  QCOMPARE(QDir(normalThumbs).entryList(QDir::Files | QDir::NoDotAndDotDot).size(), 1);
}
