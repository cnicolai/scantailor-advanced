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
#include <ProjectPages.h>
#include <ThumbnailPixmapCache.h>
#include <filters/fix_orientation/Task.h>

#include <QCheckBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
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
