// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_OUTPUTGENERATORPIXELOPS_H_
#define SCANTAILOR_OUTPUT_OUTPUTGENERATORPIXELOPS_H_

#include "OutputGenerator.h"

#include <AdjustBrightness.h>
#include <Binarize.h>
#include <BlackOnWhiteEstimator.h>
#include <ConnCompEraser.h>
#include <ConnectivityMap.h>
#include <Constants.h>
#include <CylindricalSurfaceDewarper.h>
#include <Despeckle.h>
#include <DewarpingPointMapper.h>
#include <DistortionModelBuilder.h>
#include <DrawOver.h>
#include <GrayRasterOp.h>
#include <Grayscale.h>
#include <InfluenceMap.h>
#include <Morphology.h>
#include <OrthogonalRotation.h>
#include <PolygonRasterizer.h>
#include <PolynomialSurface.h>
#include <RasterDewarper.h>
#include <RasterOp.h>
#include <SavGolFilter.h>
#include <Scale.h>
#include <SeedFill.h>
#include <TextLineTracer.h>
#include <TopBottomEdgeTracer.h>
#include <Transform.h>
#include <core/ApplicationSettings.h>
#include <imageproc/BackgroundColorCalculator.h>
#include <imageproc/ColorSegmenter.h>
#include <imageproc/ImageCombination.h>
#include <imageproc/OrthogonalRotation.h>
#include <imageproc/PolygonUtils.h>
#include <imageproc/Posterizer.h>
#include <imageproc/SkewFinder.h>
#include <imageproc/WienerFilter.h>

#include <QColor>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QPolygonF>
#include <QSize>
#include <QTransform>
#include <boost/bind/bind.hpp>
#include <boost/function.hpp>
#include <cmath>
#include <stdexcept>

#include "ColorParams.h"
#include "DebugImages.h"
#include "DewarpingOptions.h"
#include "Dpm.h"
#include "EstimateBackground.h"
#include "FillColorProperty.h"
#include "FilterData.h"
#include "ForegroundType.h"
#include "OutputImageBuilder.h"
#include "OutputProcessingParams.h"
#include "Params.h"
#include "PictureLayerProperty.h"
#include "PictureShapeOptions.h"
#include "RenderParams.h"
#include "Settings.h"
#include "TaskStatus.h"
#include "Utils.h"
#include "ZoneCategoryProperty.h"

using namespace imageproc;
using namespace dewarping;

namespace output {
namespace {
template <typename PixelType>
PixelType reserveBlackAndWhite(PixelType color);

template <>
uint32_t reserveBlackAndWhite(uint32_t color) {
  // We handle both RGB32 and ARGB32 here.
  switch (color & 0x00FFFFFF) {
    case 0x00000000:
      return 0xFF010101;
    case 0x00FFFFFF:
      return 0xFFFEFEFE;
    default:
      return color;
  }
}

template <>
uint8_t reserveBlackAndWhite(uint8_t color) {
  switch (color) {
    case 0x00:
      return 0x01;
    case 0xFF:
      return 0xFE;
    default:
      return color;
  }
}

template <typename PixelType>
void reserveBlackAndWhite(QImage& img) {
  const int width = img.width();
  const int height = img.height();

  auto* imageLine = reinterpret_cast<PixelType*>(img.bits());
  const int imageStride = img.bytesPerLine() / sizeof(PixelType);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      imageLine[x] = reserveBlackAndWhite<PixelType>(imageLine[x]);
    }
    imageLine += imageStride;
  }
}

void reserveBlackAndWhite(QImage& img) {
  switch (img.format()) {
    case QImage::Format_Indexed8:
      reserveBlackAndWhite<uint8_t>(img);
      break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
      reserveBlackAndWhite<uint32_t>(img);
      break;
    default:
      throw std::invalid_argument("reserveBlackAndWhite: wrong image format.");
  }
}

template <typename PixelType>
void reserveBlackAndWhite(QImage& img, const BinaryImage& mask) {
  const int width = img.width();
  const int height = img.height();

  auto* imageLine = reinterpret_cast<PixelType*>(img.bits());
  const int imageStride = img.bytesPerLine() / sizeof(PixelType);
  const uint32_t* maskLine = mask.data();
  const int maskStride = mask.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (maskLine[x >> 5] & (msb >> (x & 31))) {
        imageLine[x] = reserveBlackAndWhite<PixelType>(imageLine[x]);
      }
    }
    imageLine += imageStride;
    maskLine += maskStride;
  }
}

void reserveBlackAndWhite(QImage& img, const BinaryImage& mask) {
  switch (img.format()) {
    case QImage::Format_Indexed8:
      reserveBlackAndWhite<uint8_t>(img, mask);
      break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
      reserveBlackAndWhite<uint32_t>(img, mask);
      break;
    default:
      throw std::invalid_argument("reserveBlackAndWhite: wrong image format.");
  }
}

template <typename MixedPixel>
void fillExcept(QImage& image, const BinaryImage& bwMask, const QColor& color) {
  auto* imageLine = reinterpret_cast<MixedPixel*>(image.bits());
  const int imageStride = image.bytesPerLine() / sizeof(MixedPixel);
  const uint32_t* bwMaskLine = bwMask.data();
  const int bwMaskStride = bwMask.wordsPerLine();
  const int width = image.width();
  const int height = image.height();
  const uint32_t msb = uint32_t(1) << 31;
  const auto fillingPixel = static_cast<MixedPixel>(color.rgba());

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (!(bwMaskLine[x >> 5] & (msb >> (x & 31)))) {
        imageLine[x] = fillingPixel;
      }
    }
    imageLine += imageStride;
    bwMaskLine += bwMaskStride;
  }
}

void fillExcept(BinaryImage& image, const BinaryImage& bwMask, const BWColor color) {
  uint32_t* imageLine = image.data();
  const int imageStride = image.wordsPerLine();
  const uint32_t* bwMaskLine = bwMask.data();
  const int bwMaskStride = bwMask.wordsPerLine();
  const int width = image.width();
  const int height = image.height();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (!(bwMaskLine[x >> 5] & (msb >> (x & 31)))) {
        if (color == BLACK) {
          imageLine[x >> 5] |= (msb >> (x & 31));
        } else {
          imageLine[x >> 5] &= ~(msb >> (x & 31));
        }
      }
    }
    imageLine += imageStride;
    bwMaskLine += bwMaskStride;
  }
}

namespace {
// Clip content polygon to image rect to avoid exception when dewarp pushes content outside (issue #30).
QPolygonF clipContentPolyToRect(const QPolygonF& contentPoly, const QRect& imageRect) {
  const QRectF r(imageRect);
  QPolygonF rectPoly;
  rectPoly << r.topLeft() << r.topRight() << r.bottomRight() << r.bottomLeft();
  return contentPoly.intersected(rectPoly);
}
}  // namespace

void fillMarginsInPlace(QImage& image,
                        const QPolygonF& contentPoly,
                        const QColor& color,
                        const bool antialiasing = true) {
  const QRect imageRect = image.rect();
  QPolygonF clipped = clipContentPolyToRect(contentPoly, imageRect);
  if (clipped.isEmpty() || clipped.boundingRect().isEmpty()) {
    image.fill(color);
    return;
  }

  if ((image.format() == QImage::Format_Mono) || (image.format() == QImage::Format_MonoLSB)) {
    BinaryImage binaryImage(image);
    PolygonRasterizer::fillExcept(binaryImage, (color == Qt::black) ? BLACK : WHITE, clipped, Qt::WindingFill);
    image = binaryImage.toQImage();
    return;
  }
  if ((image.format() == QImage::Format_Indexed8) && image.isGrayscale()) {
    PolygonRasterizer::grayFillExcept(image, static_cast<unsigned char>(qGray(color.rgb())), clipped, Qt::WindingFill);
    return;
  }

  assert(image.format() == QImage::Format_RGB32 || image.format() == QImage::Format_ARGB32);

  const QImage::Format imageFormat = image.format();
  image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  {
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);

    QPainterPath outerPath;
    outerPath.addRect(image.rect());
    QPainterPath innerPath;
    innerPath.addPolygon(PolygonUtils::round(clipped));

    painter.drawPath(outerPath.subtracted(innerPath));
  }
  image = image.convertToFormat(imageFormat);
}

void fillMarginsInPlace(BinaryImage& image, const QPolygonF& contentPoly, const BWColor color) {
  const QRect imageRect = image.rect();
  QPolygonF clipped = clipContentPolyToRect(contentPoly, imageRect);
  if (clipped.isEmpty() || clipped.boundingRect().isEmpty()) {
    image.fill(color == BLACK ? BLACK : WHITE);
    return;
  }

  PolygonRasterizer::fillExcept(image, color, clipped, Qt::WindingFill);
}

void fillMarginsInPlace(BinaryImage& image, const BinaryImage& contentMask, const BWColor color) {
  if (image.size() != contentMask.size()) {
    throw std::invalid_argument("fillMarginsInPlace: img and mask have different sizes");
  }

  fillExcept(image, contentMask, color);
}

void fillMarginsInPlace(QImage& image, const BinaryImage& contentMask, const QColor& color) {
  if (image.size() != contentMask.size()) {
    throw std::invalid_argument("fillMarginsInPlace: img and mask have different sizes");
  }

  if ((image.format() == QImage::Format_Mono) || (image.format() == QImage::Format_MonoLSB)) {
    BinaryImage binaryImage(image);
    fillExcept(binaryImage, contentMask, (color == Qt::black) ? BLACK : WHITE);
    image = binaryImage.toQImage();
    return;
  }

  if ((image.format() == QImage::Format_Indexed8) && image.isGrayscale()) {
    fillExcept<uint8_t>(image, contentMask, color);
  } else {
    assert(image.format() == QImage::Format_RGB32 || image.format() == QImage::Format_ARGB32);
    fillExcept<uint32_t>(image, contentMask, color);
  }
}

void applyFillZonesInPlace(QImage& img,
                           const ZoneSet& zones,
                           const boost::function<QPointF(const QPointF&)>& origToOutput,
                           bool antialiasing = true) {
  if (zones.empty()) {
    return;
  }

  QImage canvas(img.convertToFormat(QImage::Format_ARGB32_Premultiplied));
  {
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing, antialiasing);
    painter.setPen(Qt::NoPen);

    for (const Zone& zone : zones) {
      const QColor color(zone.properties().locateOrDefault<FillColorProperty>()->color());
      const QPolygonF poly(zone.spline().transformed(origToOutput).toPolygon());
      painter.setBrush(color);
      painter.drawPolygon(poly, Qt::WindingFill);
    }
  }

  if ((img.format() == QImage::Format_Indexed8) && img.isGrayscale()) {
    img = toGrayscale(canvas);
  } else {
    img = canvas.convertToFormat(img.format());
  }
}

using MapPointFunc = QPointF (QTransform::*)(const QPointF&) const;

void applyFillZonesInPlace(QImage& img, const ZoneSet& zones, const QTransform& transform, bool antialiasing = true) {
  applyFillZonesInPlace(img, zones,
                        boost::bind(static_cast<MapPointFunc>(&QTransform::map), transform, boost::placeholders::_1),
                        antialiasing);
}

void applyFillZonesInPlace(BinaryImage& img,
                           const ZoneSet& zones,
                           const boost::function<QPointF(const QPointF&)>& origToOutput) {
  if (zones.empty()) {
    return;
  }

  for (const Zone& zone : zones) {
    const QColor color(zone.properties().locateOrDefault<FillColorProperty>()->color());
    const BWColor bwColor = qGray(color.rgb()) < 128 ? BLACK : WHITE;
    const QPolygonF poly(zone.spline().transformed(origToOutput).toPolygon());
    PolygonRasterizer::fill(img, bwColor, poly, Qt::WindingFill);
  }
}

void applyFillZonesInPlace(BinaryImage& img, const ZoneSet& zones, const QTransform& transform) {
  applyFillZonesInPlace(img, zones,
                        boost::bind(static_cast<MapPointFunc>(&QTransform::map), transform, boost::placeholders::_1));
}

void applyFillZonesToMixedInPlace(QImage& img,
                                  const ZoneSet& zones,
                                  const boost::function<QPointF(const QPointF&)>& origToOutput,
                                  const BinaryImage& pictureMask,
                                  bool binaryMode) {
  if (binaryMode) {
    BinaryImage bwContent(img, BinaryThreshold(1));
    applyFillZonesInPlace(bwContent, zones, origToOutput);
    applyFillZonesInPlace(img, zones, origToOutput);
    combineImages(img, bwContent, pictureMask);
  } else {
    QImage content(img);
    applyMask(content, pictureMask);
    applyFillZonesInPlace(content, zones, origToOutput, false);
    applyFillZonesInPlace(img, zones, origToOutput);
    combineImages(img, content, pictureMask);
  }
}

void applyFillZonesToMixedInPlace(QImage& img,
                                  const ZoneSet& zones,
                                  const QTransform& transform,
                                  const BinaryImage& pictureMask,
                                  bool binaryMode) {
  applyFillZonesToMixedInPlace(
      img, zones, boost::bind(static_cast<MapPointFunc>(&QTransform::map), transform, boost::placeholders::_1),
      pictureMask, binaryMode);
}

void applyFillZonesToMask(BinaryImage& mask,
                          const ZoneSet& zones,
                          const boost::function<QPointF(const QPointF&)>& origToOutput,
                          const BWColor fillColor = BLACK) {
  if (zones.empty()) {
    return;
  }

  for (const Zone& zone : zones) {
    const QPolygonF poly(zone.spline().transformed(origToOutput).toPolygon());
    PolygonRasterizer::fill(mask, fillColor, poly, Qt::WindingFill);
  }
}

void applyFillZonesToMask(BinaryImage& mask,
                          const ZoneSet& zones,
                          const QTransform& transform,
                          const BWColor fillColor = BLACK) {
  applyFillZonesToMask(mask, zones, boost::bind((MapPointFunc) &QTransform::map, transform, boost::placeholders::_1),
                       fillColor);
}

QImage smoothToGrayscale(const QImage& src, const Dpi& dpi) {
  const int minDpi = std::min(dpi.horizontal(), dpi.vertical());
  int window;
  int degree;
  if (minDpi <= 200) {
    window = 5;
    degree = 3;
  } else if (minDpi <= 400) {
    window = 7;
    degree = 4;
  } else if (minDpi <= 800) {
    window = 11;
    degree = 4;
  } else {
    window = 11;
    degree = 2;
  }
  return savGolFilter(src, QSize(window, window), degree, degree);
}
}  // namespace
}  // namespace output

#endif  // SCANTAILOR_OUTPUT_OUTPUTGENERATORPIXELOPS_H_
