// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_core_qt.h"

#include <ForgedImages.h>
#include <ImageLoader.h>
#include <ImageMetadata.h>
#include <ImageMetadataLoader.h>
#include <ImageSizeLimit.h>
#include <ProcessMemory.h>
#include <SmartFilenameOrdering.h>
#include <Units.h>
#include <foundation/Utils.h>

#include <QBuffer>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QSignalSpy>
#include <QString>
#include <QtTest/QtTest>
#include <vector>

namespace {

constexpr qint64 kFootprintBoundBytes = qint64(64) * 1024 * 1024;
constexpr qint64 kTimeBoundMs = 1000;

// ImageLoader and the metadata loaders close the device they are given, so every call gets a fresh buffer.
QImage loadFrom(const QByteArray& bytes) {
  QBuffer buffer;
  buffer.setData(bytes);
  if (!buffer.open(QIODevice::ReadOnly)) {
    return QImage();
  }
  return ImageLoader::load(buffer, 0);
}

// Refusing the forged file must return a null image quickly and without growing the peak footprint.
void verifyRefusedWithoutAllocating(const QByteArray& bytes) {
  QVERIFY(!bytes.isEmpty());
  const qint64 peakBefore = test_support::peakPhysFootprintBytes();
  QVERIFY(peakBefore >= 0);
  QElapsedTimer timer;
  timer.start();
  const QImage image = loadFrom(bytes);
  const qint64 elapsedMs = timer.elapsed();
  const qint64 peakAfter = test_support::peakPhysFootprintBytes();

  QVERIFY(image.isNull());
  QVERIFY2(elapsedMs < kTimeBoundMs, qPrintable(QString::number(elapsedMs) + " ms"));
  QVERIFY2(peakAfter - peakBefore < kFootprintBoundBytes, qPrintable(QString::number(peakAfter - peakBefore)));
}

// Header-only metadata of a file, with the loader status.
ImageMetadataLoader::Status headerSize(const QByteArray& bytes, QSize* size) {
  QBuffer buffer;
  buffer.setData(bytes);
  if (!buffer.open(QIODevice::ReadOnly)) {
    return ImageMetadataLoader::GENERIC_ERROR;
  }
  return ImageMetadataLoader::load(buffer, [size](const ImageMetadata& metadata) { *size = metadata.size(); });
}

}  // namespace

void TestCoreQt::testUnitsToString() {
  QCOMPARE(unitsToString(PIXELS), QString("px"));
  QCOMPARE(unitsToString(MILLIMETRES), QString("mm"));
  QCOMPARE(unitsToString(CENTIMETRES), QString("cm"));
  QCOMPARE(unitsToString(INCHES), QString("in"));
}

void TestCoreQt::testUnitsFromString() {
  QCOMPARE(unitsFromString(QString("px")), PIXELS);
  QCOMPARE(unitsFromString(QString("mm")), MILLIMETRES);
  QCOMPARE(unitsFromString(QString("cm")), CENTIMETRES);
  QCOMPARE(unitsFromString(QString("in")), INCHES);
  QCOMPARE(unitsFromString(QString("unknown")), MILLIMETRES);
}

void TestCoreQt::testUnitsToLocalizedString() {
  QString px = unitsToLocalizedString(PIXELS);
  QString mm = unitsToLocalizedString(MILLIMETRES);
  QVERIFY(!px.isEmpty());
  QVERIFY(!mm.isEmpty());
}

void TestCoreQt::testUtilsDoubleToString() {
  QCOMPARE(foundation::Utils::doubleToString(0.0), QString("0"));
  QVERIFY(foundation::Utils::doubleToString(123.456).contains("123"));
  QVERIFY(foundation::Utils::doubleToString(-42.5).startsWith("-"));
}

void TestCoreQt::testSmartFilenameOrdering() {
  const SmartFilenameOrdering less;
  const QFileInfo a("/etc/1.png");
  const QFileInfo b("/etc/2.png");
  QVERIFY(less(a, b));
  QVERIFY(!less(b, a));
  QVERIFY(!less(a, a));

  const QFileInfo c("/etc/a_0002.png");
  const QFileInfo d("/etc/a_1.png");
  QVERIFY(less(d, c));
}

void TestCoreQt::testSignalSpy() {
  Emitter obj;
  QSignalSpy spy(&obj, &Emitter::valueChanged);
  QVERIFY(spy.isValid());
  QCOMPARE(spy.count(), 0);

  obj.doEmit(QString("hello"));
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toString(), QString("hello"));

  obj.doEmit(QString("world"));
  QCOMPARE(spy.count(), 2);
  QCOMPARE(spy.at(1).at(0).toString(), QString("world"));
}

void TestCoreQt::testForgedPngIsRefused() {
  // The header claims 100000 x 100000 px in a file of a few dozen bytes; Qt reports that size before reading.
  verifyRefusedWithoutAllocating(test_support::forgedPng(100000, 100000));
}

void TestCoreQt::testForgedJpegIsRefusedAndLargeJpegLoads() {
  // Progressive: the harmful kind, whose decompression allocates the whole coefficient image.
  verifyRefusedWithoutAllocating(test_support::forgedJpeg(65500, 65500, true));

  // 72 MP, above the 64 Mi px that Qt's default limit refuses.
  QImage large(9000, 8000, QImage::Format_Grayscale8);
  large.fill(Qt::white);
  QByteArray bytes;
  {
    QBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QImageWriter writer(&buffer, "JPEG");
    QVERIFY(writer.write(large));
  }
  const QImage loaded = loadFrom(bytes);
  QVERIFY(!loaded.isNull());
  QCOMPARE(loaded.size(), QSize(9000, 8000));
}

void TestCoreQt::testLargeGreyPngLoads() {
  QImage large(9000, 8000, QImage::Format_Grayscale8);
  large.fill(Qt::white);
  QByteArray bytes;
  {
    QBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QImageWriter writer(&buffer, "PNG");
    QVERIFY(writer.write(large));
  }

  const QImage loaded = loadFrom(bytes);
  QVERIFY(!loaded.isNull());
  QCOMPARE(loaded.size(), QSize(9000, 8000));
  // The loader sets the limit once; it is the project ceiling and never 0.
  QCOMPARE(QImageReader::allocationLimit(), ImageSizeLimit::kQtAllocationLimitMb);

  // Control: with Qt's default limit the same bytes are refused, so the loader's limit is what lets them load.
  QImageReader::setAllocationLimit(256);
  {
    QBuffer buffer;
    buffer.setData(bytes);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QImageReader reader(&buffer);
    QImage control;
    const bool read = reader.read(&control);
    QImageReader::setAllocationLimit(ImageSizeLimit::kQtAllocationLimitMb);
    QVERIFY2(!read, "the control read must fail under Qt's default limit");
  }
  QCOMPARE(QImageReader::allocationLimit(), ImageSizeLimit::kQtAllocationLimitMb);

  // Qt's limit refuses the forged 100000 x 100000 px by itself, so the size check of ImageLoader is pinned by a
  // real file that Qt would load: 65536 px per side is one over the limit and costs only 64 KB.
  for (const QSize& side : {QSize(65535, 1), QSize(65536, 1), QSize(1, 65536)}) {
    QImage line(side, QImage::Format_Grayscale8);
    line.fill(Qt::white);
    QByteArray lineBytes;
    {
      QBuffer buffer(&lineBytes);
      QVERIFY(buffer.open(QIODevice::WriteOnly));
      QImageWriter writer(&buffer, "PNG");
      QVERIFY(writer.write(line));
    }
    const QImage result = loadFrom(lineBytes);
    QCOMPARE(!result.isNull(), !ImageSizeLimit::exceeds(side));
  }
}

void TestCoreQt::testForgedPngMetadataReportsClaimedSize() {
  // One verdict for all formats: the loader reports the claimed size and the one consumer check refuses it.
  QSize size;
  QCOMPARE(headerSize(test_support::forgedPng(100000, 100000), &size), ImageMetadataLoader::LOADED);
  QCOMPARE(size, QSize(100000, 100000));
  QVERIFY(ImageSizeLimit::exceeds(size));

  // Above libpng's default limit of 1000000 px per side: without the lifted limit this is a generic error.
  size = QSize();
  QCOMPARE(headerSize(test_support::forgedPng(2000000, 1), &size), ImageMetadataLoader::LOADED);
  QCOMPARE(size, QSize(2000000, 1));
  QVERIFY(ImageSizeLimit::exceeds(size));
}
