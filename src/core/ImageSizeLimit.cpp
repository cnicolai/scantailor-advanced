// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageSizeLimit.h"

#include <QtGlobal>
#include <vector>

#include "ImageId.h"
#include "ImageMetadata.h"
#include "ImageMetadataLoader.h"

bool ImageSizeLimit::exceeds(const QSize& size) {
  const int width = size.width();
  const int height = size.height();
  if (width < 0 || height < 0) {
    return true;
  }
  return width > kMaxSide || height > kMaxSide || qint64(width) * height > kMaxPixels;
}

QString ImageSizeLimit::sizeText(const QSize& size) {
  return tr("%1 × %2 px").arg(QString::number(quint32(size.width()))).arg(QString::number(quint32(size.height())));
}

QString ImageSizeLimit::limitText() {
  return tr("%1 px per side and %2 megapixels in total").arg(kMaxSide).arg(qRound64(kMaxPixels / 1e6));
}

QString ImageSizeLimit::tooLargeText(const QSize& size) {
  return tr("too large: %1").arg(sizeText(size));
}

bool ImageSizeLimit::claimedSizeIfTooLarge(const ImageId& imageId, QSize* claimedSize) {
  std::vector<ImageMetadata> pages;
  const ImageMetadataLoader::Status status = ImageMetadataLoader::load(
      imageId.filePath(), [&pages](const ImageMetadata& metadata) { pages.push_back(metadata); });
  if (status != ImageMetadataLoader::LOADED) {
    return false;
  }

  const size_t index = size_t(imageId.zeroBasedPage());
  if (index >= pages.size() || !exceeds(pages[index].size())) {
    return false;
  }
  *claimedSize = pages[index].size();
  return true;
}

QString ImageSizeLimit::oversizedPagesText(const std::vector<ImageMetadata>& pages) {
  for (size_t i = 0; i < pages.size(); ++i) {
    const QSize size = pages[i].size();
    if (!exceeds(size)) {
      continue;
    }
    if (pages.size() == 1) {
      return tooLargeText(size);
    }
    return tr("page %1 %2").arg(int(i + 1)).arg(tooLargeText(size));
  }
  return QString();
}
