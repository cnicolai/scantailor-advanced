// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_HIDPI_H_
#define SCANTAILOR_CORE_HIDPI_H_

#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTransform>

/*
 * The buffer rule for everything that is painted onto a screen:
 *
 * View-space coordinates stay logical. A buffer built for the screen is the logical size times the device pixel
 * ratio in pixels, carries setDevicePixelRatio(), and is positioned and clipped in logical coordinates. The ratio
 * comes from the widget (devicePixelRatioF()) or from painter->device(), never from a constant such as 2.
 * Pixmaps that are scaled through their own pixel size (the downscaled preview, the loaded thumbnail) stay at
 * ratio 1.
 *
 * At ratio 1 every function below returns exactly what the code computed before the Retina work.
 */
namespace hidpi {

/// dpr if it is finite and greater than 0, otherwise 1.0.
double sanitizedRatio(double dpr);

/// Device-pixel size of a logical size: per side 0 for a side <= 0, otherwise at least 1 (qRound of side * ratio).
QSize deviceSize(const QSize& logical, double dpr);

/// Logical-to-device transform: logicalXform followed by a scale by the ratio.
QTransform toDevice(const QTransform& logicalXform, double dpr);

/// Device-pixel rect that the transformed source rect covers inside a viewport of the given logical size.
QRect targetRect(const QRectF& sourceRect, const QTransform& logicalXform, const QSize& logicalViewport, double dpr);

/// Logical position of an origin given in device pixels.
QPointF toLogical(const QPoint& deviceOrigin, double dpr);

/// Pixmap of the logical size times the ratio in pixels, with the ratio set.
QPixmap makePixmap(const QSize& logicalSize, double dpr);

/// Logical (device independent) size of a pixmap.
QSizeF logicalSize(const QPixmap& pixmap);

/// Logical rect of a pixmap, anchored at the origin.
QRectF logicalRect(const QPixmap& pixmap);

}  // namespace hidpi

#endif
