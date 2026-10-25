// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "HiDpi.h"

#include <algorithm>
#include <cmath>

namespace hidpi {

double sanitizedRatio(const double dpr) {
  return (std::isfinite(dpr) && dpr > 0.0) ? dpr : 1.0;
}

namespace {

int deviceSide(const int logical, const double ratio) {
  if (logical <= 0) {
    return 0;
  }
  return std::max(1, qRound(logical * ratio));
}

}  // namespace

QSize deviceSize(const QSize& logical, const double dpr) {
  const double ratio = sanitizedRatio(dpr);
  return QSize(deviceSide(logical.width(), ratio), deviceSide(logical.height(), ratio));
}

QTransform toDevice(const QTransform& logicalXform, const double dpr) {
  const double ratio = sanitizedRatio(dpr);
  return logicalXform * QTransform::fromScale(ratio, ratio);
}

QRect targetRect(const QRectF& sourceRect,
                 const QTransform& logicalXform,
                 const QSize& logicalViewport,
                 const double dpr) {
  return toDevice(logicalXform, dpr)
      .map(sourceRect)
      .boundingRect()
      .toRect()
      .intersected(QRect(QPoint(0, 0), deviceSize(logicalViewport, dpr)));
}

QPointF toLogical(const QPoint& deviceOrigin, const double dpr) {
  return QPointF(deviceOrigin) / sanitizedRatio(dpr);
}

QPixmap makePixmap(const QSize& logicalSize, const double dpr) {
  QPixmap pixmap(deviceSize(logicalSize, dpr));
  pixmap.setDevicePixelRatio(sanitizedRatio(dpr));
  return pixmap;
}

QSizeF logicalSize(const QPixmap& pixmap) {
  return pixmap.deviceIndependentSize();
}

QRectF logicalRect(const QPixmap& pixmap) {
  return QRectF(QPointF(0, 0), logicalSize(pixmap));
}

}  // namespace hidpi
