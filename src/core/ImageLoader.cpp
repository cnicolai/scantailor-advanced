// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageLoader.h"

#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QtGui/QImageReader>
#include <mutex>

#include "ImageId.h"
#include "ImageSizeLimit.h"
#include "TiffReader.h"

namespace {
std::once_flag qtReaderLimitFlag;
}

QImage ImageLoader::load(const ImageId& imageId) {
  return load(imageId.filePath(), imageId.zeroBasedPage());
}

QImage ImageLoader::load(const QString& filePath, const int pageNum) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return QImage();
  }
  return load(file, pageNum);
}

QImage ImageLoader::load(QIODevice& ioDev, const int pageNum) {
  return loadImpl(ioDev, pageNum, ImageSizeLimit::LimitKind::OWN_FILE);
}

QImage ImageLoader::loadSource(const ImageId& imageId) {
  QFile file(imageId.filePath());
  if (!file.open(QIODevice::ReadOnly)) {
    return QImage();
  }
  return loadSource(file, imageId.zeroBasedPage());
}

QImage ImageLoader::loadSource(QIODevice& ioDev, const int pageNum) {
  return loadImpl(ioDev, pageNum, ImageSizeLimit::LimitKind::SOURCE);
}

QImage ImageLoader::loadImpl(QIODevice& ioDev, const int pageNum, const ImageSizeLimit::LimitKind limit) {
  // Qt's default limit of 256 MB refuses every image above 64 Mi px. It is raised once to the project ceiling
  // (the source path checks the claimed size below, so Qt never sees a larger source image) and never set to 0, which
  // would let Qt allocate the claimed size before failing. The thumbnail thread and the worker threads all come
  // through here.
  std::call_once(qtReaderLimitFlag, [] { QImageReader::setAllocationLimit(ImageSizeLimit::kQtAllocationLimitMb); });

  if (TiffReader::canRead(ioDev)) {
    return TiffReader::readImage(ioDev, pageNum, limit);
  }

  if (pageNum != 0) {
    // Qt can only load the first page of multi-page images.
    return QImage();
  }

  QImage image;
  QImageReader reader(&ioDev);
  reader.setAutoTransform(true);  // Helps with orientation and some formats (issue #20).
  // Issue #98: on some builds Qt may not detect JPG format; force it for .jpg/.jpeg files.
  if (QFile* file = qobject_cast<QFile*>(&ioDev)) {
    const QString ext = QFileInfo(file->fileName()).suffix().toLower();
    if (ext == QStringLiteral("jpg") || ext == QStringLiteral("jpeg")) {
      reader.setFormat("jpeg");
    }
  }
  // The header of a forged file can claim any size; refuse it before the reader allocates it. An invalid size
  // means the format cannot tell, and the read decides as before.
  const QSize claimed = reader.size();
  if (claimed.isValid() && ImageSizeLimit::exceeds(claimed, limit)) {
    return QImage();
  }
  reader.read(&image);
  return image;
}
