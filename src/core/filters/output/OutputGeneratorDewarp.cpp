// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutputGenerator.h"

#include <AdjustBrightness.h>
#include <Constants.h>
#include <CylindricalSurfaceDewarper.h>
#include <DewarpingPointMapper.h>
#include <DistortionModelBuilder.h>
#include <RasterDewarper.h>
#include <RasterOp.h>
#include <TextLineTracer.h>
#include <TopBottomEdgeTracer.h>
#include <Transform.h>
#include <imageproc/BackgroundColorCalculator.h>
#include <imageproc/OrthogonalRotation.h>
#include <imageproc/WienerFilter.h>

#include <QColor>
#include <QPointF>
#include <QPolygonF>
#include <QSize>
#include <QTransform>
#include <boost/bind/bind.hpp>
#include <boost/function.hpp>
#include <cmath>
#include <stdexcept>

#include "OutputGeneratorPixelOps.h"
#include "OutputGeneratorProcessor.h"
#include "OutputImageBuilder.h"
#include "Utils.h"

using namespace imageproc;
using namespace dewarping;

namespace output {
namespace {
void applyAffineTransform(QImage& image, const QTransform& xform, const QColor& outsideColor) {
  if (xform.isIdentity()) {
    return;
  }
  image = transform(image, xform, image.rect(), OutsidePixels::assumeWeakColor(outsideColor));
}

void applyAffineTransform(BinaryImage& image, const QTransform& xform, const BWColor outsideColor) {
  if (xform.isIdentity()) {
    return;
  }
  const QColor color = (outsideColor == BLACK) ? Qt::black : Qt::white;
  QImage converted = image.toQImage();
  applyAffineTransform(converted, xform, color);
  image = BinaryImage(converted);
}

void movePointToTopMargin(BinaryImage& bwImage, XSpline& spline, int idx) {
  QPointF pos = spline.controlPointPosition(idx);

  for (int j = 0; j < pos.y(); j++) {
    if (bwImage.getPixel(static_cast<int>(pos.x()), j) == WHITE) {
      int count = 0;
      int checkNum = 16;

      for (int jj = j; jj < (j + checkNum); jj++) {
        if (bwImage.getPixel(static_cast<int>(pos.x()), jj) == WHITE) {
          count++;
        }
      }

      if (count == checkNum) {
        pos.setY(j);
        spline.moveControlPoint(idx, pos);
        break;
      }
    }
  }
}

void movePointToBottomMargin(BinaryImage& bwImage, XSpline& spline, int idx) {
  QPointF pos = spline.controlPointPosition(idx);

  for (int j = bwImage.height() - 1; j > pos.y(); j--) {
    if (bwImage.getPixel(static_cast<int>(pos.x()), j) == WHITE) {
      int count = 0;
      int checkNum = 16;

      for (int jj = j; jj > (j - checkNum); jj--) {
        if (bwImage.getPixel(static_cast<int>(pos.x()), jj) == WHITE) {
          count++;
        }
      }

      if (count == checkNum) {
        pos.setY(j);

        spline.moveControlPoint(idx, pos);

        break;
      }
    }
  }
}

void drawPoint(QImage& image, const QPointF& pt) {
  QPoint pts = pt.toPoint();

  for (int i = pts.x() - 10; i < pts.x() + 10; i++) {
    for (int j = pts.y() - 10; j < pts.y() + 10; j++) {
      QPoint p1(i, j);

      image.setPixel(p1, qRgb(255, 0, 0));
    }
  }
}

void movePointToTopMargin(BinaryImage& bwImage, std::vector<QPointF>& polyline, int idx) {
  QPointF& pos = polyline[idx];

  for (int j = 0; j < pos.y(); j++) {
    if (bwImage.getPixel(static_cast<int>(pos.x()), j) == WHITE) {
      int count = 0;
      int checkNum = 16;

      for (int jj = j; jj < (j + checkNum); jj++) {
        if (bwImage.getPixel(static_cast<int>(pos.x()), jj) == WHITE) {
          count++;
        }
      }

      if (count == checkNum) {
        pos.setY(j);

        break;
      }
    }
  }
}

void movePointToBottomMargin(BinaryImage& bwImage, std::vector<QPointF>& polyline, int idx) {
  QPointF& pos = polyline[idx];

  for (int j = bwImage.height() - 1; j > pos.y(); j--) {
    if (bwImage.getPixel(static_cast<int>(pos.x()), j) == WHITE) {
      int count = 0;
      int checkNum = 16;

      for (int jj = j; jj > (j - checkNum); jj--) {
        if (bwImage.getPixel(static_cast<int>(pos.x()), jj) == WHITE) {
          count++;
        }
      }

      if (count == checkNum) {
        pos.setY(j);

        break;
      }
    }
  }
}

inline float vertBorderSkewAngle(const QPointF& top, const QPointF& bottom) {
  return static_cast<float>(
      std::abs(std::atan((bottom.x() - top.x()) / (bottom.y() - top.y())) * 180.0 / constants::PI));
}
}  // namespace

std::unique_ptr<OutputImage> OutputGenerator::Processor::processWithDewarping(ZoneSet& pictureZones,
                                                                              const ZoneSet& fillZones,
                                                                              DistortionModel& distortionModel,
                                                                              const DepthPerception& depthPerception,
                                                                              BinaryImage* autoPictureMask,
                                                                              BinaryImage* specklesImage) {
  OutputImageBuilder imageBuilder;

  // The output we would get if dewarping was turned off, except always grayscale.
  // Used for automatic picture detection and binarization threshold calculation.
  // This image corresponds to the area of workingBoundingRect.
  GrayImage warpedGrayOutput;
  if (!m_renderParams.normalizeIllumination()) {
    warpedGrayOutput = transformToGray(m_inputGrayImage, m_xform.transform(), m_workingBoundingRect,
                                       OutsidePixels::assumeWeakColor(m_outsideBackgroundColor));
  } else {
    warpedGrayOutput = normalizeIlluminationGray(m_inputGrayImage, m_preCropAreaInOriginalCs, m_xform.transform(),
                                                 m_workingBoundingRect);
  }

  // Original image, but:
  // 1. In a format we can handle, that is grayscale, RGB32, ARGB32
  // 2. With illumination normalized over the content area, if required.
  // 3. With margins filled with white, if required.
  QImage normalizedOriginal;
  const QTransform workingToOrig
      = QTransform().translate(m_workingBoundingRect.left(), m_workingBoundingRect.top()) * m_xform.transformBack();
  if (!m_renderParams.normalizeIllumination()) {
    normalizedOriginal = (m_colorOriginal) ? m_inputOrigImage : m_inputGrayImage;
  } else {
    GrayImage normalizedGray = transformToGray(warpedGrayOutput, workingToOrig, m_inputOrigImage.rect(),
                                               OutsidePixels::assumeWeakColor(m_outsideBackgroundColor));
    if (!m_colorOriginal) {
      normalizedOriginal = normalizedGray;
    } else {
      normalizedOriginal = m_inputOrigImage;
      adjustBrightnessGrayscale(normalizedOriginal, normalizedGray);
      if (m_dbg) {
        m_dbg->add(normalizedOriginal, "norm_illum_color");
      }
    }

    m_outsideBackgroundColor
        = BackgroundColorCalculator::calcDominantBackgroundColor(normalizedOriginal, m_outCropAreaInOriginalCs);
  }

  m_status.throwIfCancelled();

  const ColorCommonOptions& colorCommonOptions = m_colorParams.colorCommonOptions();
  wienerColorFilterInPlace(normalizedOriginal,
                           QSize(colorCommonOptions.wienerWindowSize(), colorCommonOptions.wienerWindowSize()),
                           colorCommonOptions.wienerCoef());

  // Picture mask (white indicate a picture) in the same coordinates as
  // warpedGrayOutput.  Only built for Mixed mode.
  BinaryImage warpedBwMask;
  if (m_renderParams.mixedOutput()) {
    warpedBwMask = BinaryImage(m_workingBoundingRect.size(), BLACK);
    processPictureZones(warpedBwMask, pictureZones, warpedGrayOutput);
    if (m_dbg) {
      m_dbg->add(warpedBwMask, "warpedBwMask");
    }
    m_status.throwIfCancelled();

    if (autoPictureMask) {
      if (autoPictureMask->size() != m_targetSize) {
        BinaryImage(m_targetSize).swap(*autoPictureMask);
      }
      autoPictureMask->fill(BLACK);
      rasterOp<RopSrc>(*autoPictureMask, m_croppedContentRect, warpedBwMask, m_contentRectInWorkingCs.topLeft());
    }
    m_status.throwIfCancelled();

    // BW mask begin
    BinaryImage warpedBwContent;
    QImage maybeSmoothed;
    if (!m_renderParams.needSavitzkyGolaySmoothing()) {
      maybeSmoothed = warpedGrayOutput;
    } else {
      maybeSmoothed = smoothToGrayscale(warpedGrayOutput, m_dpi);
      if (m_dbg) {
        m_dbg->add(maybeSmoothed, "smoothed");
      }
      m_status.throwIfCancelled();
    }

    warpedBwContent = binarize(maybeSmoothed, m_contentAreaInWorkingCs);
    maybeSmoothed = QImage();  // Save memory.
    if (m_dbg) {
      m_dbg->add(warpedBwContent, "binarized_and_cropped");
    }
    m_status.throwIfCancelled();

    if (m_renderParams.needMorphologicalSmoothing()) {
      morphologicalSmoothInPlace(warpedBwContent);
      m_status.throwIfCancelled();
    }
    // BW mask end

    modifyBinarizationMask(warpedBwMask, warpedBwContent, m_workingBoundingRect, pictureZones);
    warpedBwContent.release();  // Save memory.
    if (m_dbg) {
      m_dbg->add(warpedBwMask, "warpedBwMask with zones");
    }
    m_status.throwIfCancelled();
  }

  if (m_dewarpingOptions.dewarpingMode() == AUTO) {
    distortionModel = buildAutoDistortionModel(warpedGrayOutput, workingToOrig);
  } else if (m_dewarpingOptions.dewarpingMode() == MARGINAL) {
    distortionModel = buildMarginalDistortionModel();
  }
  warpedGrayOutput = GrayImage();  // Save memory.

  QTransform rotateXform;
  QImage dewarped;
  try {
    dewarped = dewarp(QTransform(), normalizedOriginal, m_xform.transform(), distortionModel, depthPerception,
                      m_outsideBackgroundColor);

    const double deskewAngle = -findSkew(dewarped);
    rotateXform = Utils::rotate(deskewAngle, m_outRect);
    m_dewarpingOptions.setPostDeskewAngle(deskewAngle);
  } catch (const std::runtime_error&) {
    // Probably an impossible distortion model.  Let's fall back to a trivial one.
    setupTrivialDistortionModel(distortionModel);
    m_dewarpingOptions.setPostDeskewAngle(.0);

    dewarped = dewarp(QTransform(), normalizedOriginal, m_xform.transform(), distortionModel, depthPerception,
                      m_outsideBackgroundColor);
  }
  normalizedOriginal = QImage();  // Save memory.
  m_settings->setDewarpingOptions(m_pageId, m_dewarpingOptions);
  applyAffineTransform(dewarped, rotateXform, m_outsideBackgroundColor);
  normalizedOriginal = QImage();  // Save memory.
  if (m_dbg) {
    m_dbg->add(dewarped, "dewarped");
  }
  m_status.throwIfCancelled();

  auto mapper = std::make_shared<DewarpingPointMapper>(distortionModel, depthPerception.value(), m_xform.transform(),
                                                       m_croppedContentRect, rotateXform);
  const boost::function<QPointF(const QPointF&)> origToOutput(
      boost::bind(&DewarpingPointMapper::mapToDewarpedSpace, mapper, boost::placeholders::_1));

  BinaryImage dewarpingContentAreaMask(m_inputGrayImage.size(), BLACK);
  {
    fillMarginsInPlace(dewarpingContentAreaMask, m_contentAreaInOriginalCs, WHITE);
    dewarpingContentAreaMask = BinaryImage(dewarp(QTransform(), dewarpingContentAreaMask.toQImage(),
                                                  m_xform.transform(), distortionModel, depthPerception, Qt::white));
  }
  applyAffineTransform(dewarpingContentAreaMask, rotateXform, WHITE);

  if (m_renderParams.binaryOutput()) {
    QImage dewarpedAndMaybeSmoothed;
    // We only do smoothing if we are going to do binarization later.
    if (!m_renderParams.needSavitzkyGolaySmoothing()) {
      dewarpedAndMaybeSmoothed = dewarped;
    } else {
      dewarpedAndMaybeSmoothed = smoothToGrayscale(dewarped, m_dpi);
      if (m_dbg) {
        m_dbg->add(dewarpedAndMaybeSmoothed, "smoothed");
      }
      m_status.throwIfCancelled();
    }

    // don't destroy as it's needed for color segmentation
    if (!m_renderParams.needColorSegmentation()) {
      dewarped = QImage();
    }

    BinaryImage dewarpedBwContent = binarize(dewarpedAndMaybeSmoothed, dewarpingContentAreaMask);
    dewarpedAndMaybeSmoothed = QImage();  // Save memory.
    if (m_dbg) {
      m_dbg->add(dewarpedBwContent, "dewarpedBwContent");
    }
    m_status.throwIfCancelled();

    if (m_renderParams.needMorphologicalSmoothing()) {
      morphologicalSmoothInPlace(dewarpedBwContent);
      m_status.throwIfCancelled();
    }

    // It's important to keep despeckling the very last operation
    // affecting the binary part of the output. That's because
    // we will be reconstructing the input to this despeckling
    // operation from the final output file.
    maybeDespeckleInPlace(dewarpedBwContent, m_outRect, m_outRect, m_despeckleLevel, specklesImage, m_dpi);

    if (!m_renderParams.needColorSegmentation()) {
      if (!m_blackOnWhite) {
        dewarpedBwContent.invert();
      }

      applyFillZonesInPlace(dewarpedBwContent, fillZones, origToOutput);

      imageBuilder.setImage(dewarpedBwContent.toQImage());
    } else {
      QImage segmentedImage = segmentImage(dewarpedBwContent, dewarped);
      dewarped = QImage();
      dewarpedBwContent.release();

      if (m_renderParams.posterize()) {
        segmentedImage = posterizeImage(segmentedImage, m_outsideBackgroundColor);
      }

      if (!m_blackOnWhite) {
        segmentedImage.invertPixels();
      }

      applyFillZonesInPlace(segmentedImage, fillZones, origToOutput, false);
      if (m_dbg) {
        m_dbg->add(segmentedImage, "segmented_with_fill_zones");
      }
      m_status.throwIfCancelled();

      imageBuilder.setImage(segmentedImage);
    }
    return imageBuilder.build();
  }

  BinaryImage dewarpedBwMask;
  BinaryImage dewarpedBwContent;
  if (m_renderParams.mixedOutput()) {
    const QTransform origToWorkingCs
        = m_xform.transform() * QTransform().translate(-m_workingBoundingRect.left(), -m_workingBoundingRect.top());
    QTransform workingToOutputCs = QTransform().translate(m_workingBoundingRect.left(), m_workingBoundingRect.top());
    dewarpedBwMask = BinaryImage(dewarp(origToWorkingCs, warpedBwMask.toQImage(), workingToOutputCs, distortionModel,
                                        depthPerception, Qt::black));
    warpedBwMask.release();
    applyAffineTransform(dewarpedBwMask, rotateXform, BLACK);
    fillMarginsInPlace(dewarpedBwMask, dewarpingContentAreaMask, BLACK);
    if (m_dbg) {
      m_dbg->add(dewarpedBwMask, "dewarpedBwMask");
    }
    m_status.throwIfCancelled();

    if (m_renderParams.needBinarization()) {
      QImage dewarpedAndMaybeSmoothed;
      if (!m_renderParams.needSavitzkyGolaySmoothing()) {
        dewarpedAndMaybeSmoothed = dewarped;
      } else {
        dewarpedAndMaybeSmoothed = smoothToGrayscale(dewarped, m_dpi);
        if (m_dbg) {
          m_dbg->add(dewarpedAndMaybeSmoothed, "smoothed");
        }
        m_status.throwIfCancelled();
      }

      BinaryImage dewarpedBwMaskFilled(dewarpedBwMask);
      fillMarginsInPlace(dewarpedBwMaskFilled, dewarpingContentAreaMask, WHITE);
      dewarpedBwContent = binarize(dewarpedAndMaybeSmoothed, dewarpedBwMaskFilled);
      dewarpedBwMaskFilled.release();
      dewarpedAndMaybeSmoothed = QImage();  // Save memory.
      if (m_dbg) {
        m_dbg->add(dewarpedBwContent, "dewarpedBwContent");
      }
      m_status.throwIfCancelled();

      if (m_renderParams.needMorphologicalSmoothing()) {
        morphologicalSmoothInPlace(dewarpedBwContent);
      }

      // It's important to keep despeckling the very last operation
      // affecting the binary part of the output. That's because
      // we will be reconstructing the input to this despeckling
      // operation from the final output file.
      maybeDespeckleInPlace(dewarpedBwContent, m_outRect, m_croppedContentRect, m_despeckleLevel, specklesImage, m_dpi);

      if (!m_renderParams.normalizeIlluminationColor()) {
        m_outsideBackgroundColor = BackgroundColorCalculator::calcDominantBackgroundColor(
            m_colorOriginal ? m_inputOrigImage : m_inputGrayImage, m_outCropAreaInOriginalCs);

        {
          QImage origWithoutIllumination;
          if (m_colorOriginal) {
            origWithoutIllumination = m_inputOrigImage;
          } else {
            origWithoutIllumination = m_inputGrayImage;
          }
          dewarped = dewarp(QTransform(), origWithoutIllumination, m_xform.transform(), distortionModel,
                            depthPerception, m_outsideBackgroundColor);
        }
        applyAffineTransform(dewarped, rotateXform, m_outsideBackgroundColor);
        m_status.throwIfCancelled();
      }

      if (!m_renderParams.needColorSegmentation()) {
        if (m_renderParams.originalBackground()) {
          combineImages(dewarped, dewarpedBwContent);
        } else {
          combineImages(dewarped, dewarpedBwContent, dewarpedBwMask);
        }
      } else {
        QImage segmentedImage = segmentImage(dewarpedBwContent, dewarped);
        if (m_renderParams.posterize()) {
          segmentedImage = posterizeImage(segmentedImage, m_outsideBackgroundColor);
        }

        if (m_renderParams.originalBackground()) {
          combineImages(dewarped, segmentedImage);
        } else {
          combineImages(dewarped, segmentedImage, dewarpedBwMask);
        }
      }

      if (m_dbg) {
        m_dbg->add(dewarped, "combined");
      }

      if (m_renderParams.originalBackground()) {
        reserveBlackAndWhite(dewarped, dewarpedBwContent.inverted());
      } else {
        reserveBlackAndWhite(dewarped, dewarpedBwMask.inverted());
      }
      m_status.throwIfCancelled();
    }
  }

  if (m_renderParams.needBinarization() && !m_renderParams.originalBackground()) {
    m_outsideBackgroundColor = Qt::white;
  } else if (m_colorParams.colorCommonOptions().getFillingColor() == FILL_WHITE) {
    m_outsideBackgroundColor = m_blackOnWhite ? Qt::white : Qt::black;
  }
  fillMarginsInPlace(dewarped, dewarpingContentAreaMask, m_outsideBackgroundColor);

  if (!m_blackOnWhite) {
    dewarped.invertPixels();
  }

  if (m_renderParams.mixedOutput() && m_renderParams.needBinarization()) {
    applyFillZonesToMixedInPlace(dewarped, fillZones, origToOutput, dewarpedBwMask,
                                 !m_renderParams.needColorSegmentation());
  } else {
    applyFillZonesInPlace(dewarped, fillZones, origToOutput);
  }
  if (m_renderParams.originalBackground()) {
    applyFillZonesToMask(dewarpedBwContent, fillZones, origToOutput);
    rasterOp<RopAnd<RopSrc, RopDst>>(dewarpedBwContent, dewarpedBwMask);
  }
  if (m_dbg) {
    m_dbg->add(dewarped, "fillZones");
  }
  m_status.throwIfCancelled();

  if (m_renderParams.posterize() && !m_renderParams.needBinarization()) {
    dewarped = posterizeImage(dewarped);
  }

  if (m_renderParams.splitOutput()) {
    imageBuilder.setForegroundType(getForegroundType());
    if (!m_renderParams.originalBackground()) {
      imageBuilder.setForegroundMask(dewarpedBwMask);
    } else {
      imageBuilder.setForegroundMask(dewarpedBwContent);
      imageBuilder.setBackgroundMask(dewarpedBwMask.inverted());
    }
  }
  return imageBuilder.setImage(dewarped).build();
}

/**
 * Set up a distortion model corresponding to the content rect,
 * which will result in no distortion correction.
 */
void OutputGenerator::Processor::setupTrivialDistortionModel(DistortionModel& distortionModel) const {
  QPolygonF poly;
  if (!m_contentRect.isEmpty()) {
    poly = QRectF(m_contentRect);
  } else {
    poly << m_contentRect.topLeft() + QPointF(-0.5, -0.5);
    poly << m_contentRect.topLeft() + QPointF(0.5, -0.5);
    poly << m_contentRect.topLeft() + QPointF(0.5, 0.5);
    poly << m_contentRect.topLeft() + QPointF(-0.5, 0.5);
  }
  poly = m_xform.transformBack().map(poly);

  std::vector<QPointF> topPolyline, bottomPolyline;
  topPolyline.push_back(poly[0]);     // top-left
  topPolyline.push_back(poly[1]);     // top-right
  bottomPolyline.push_back(poly[3]);  // bottom-left
  bottomPolyline.push_back(poly[2]);  // bottom-right
  distortionModel.setTopCurve(Curve(topPolyline));
  distortionModel.setBottomCurve(Curve(bottomPolyline));
}

CylindricalSurfaceDewarper OutputGenerator::Processor::createDewarper(const DistortionModel& distortionModel,
                                                                      const QTransform& distortionModelToTarget,
                                                                      double depthPerception) {
  if (distortionModelToTarget.isIdentity()) {
    return CylindricalSurfaceDewarper(distortionModel.topCurve().polyline(), distortionModel.bottomCurve().polyline(),
                                      depthPerception);
  }

  std::vector<QPointF> topPolyline(distortionModel.topCurve().polyline());
  std::vector<QPointF> bottomPolyline(distortionModel.bottomCurve().polyline());
  for (QPointF& pt : topPolyline) {
    pt = distortionModelToTarget.map(pt);
  }
  for (QPointF& pt : bottomPolyline) {
    pt = distortionModelToTarget.map(pt);
  }
  return CylindricalSurfaceDewarper(topPolyline, bottomPolyline, depthPerception);
}

/**
 * \param origToSrc Transformation from the original image coordinates
 *                    to the coordinate system of \p src image.
 * \param srcToOutput Transformation from the \p src image coordinates
 *                      to output image coordinates.
 * \param distortionModel Distortion model.
 * \param depthPerception Depth perception.
 * \param bgColor The color to use for areas outsize of \p src.
 * \param modified_content_rect A vertically shrunk version of outputContentRect().
 *                              See function definition for more details.
 */
QImage OutputGenerator::Processor::dewarp(const QTransform& origToSrc,
                                          const QImage& src,
                                          const QTransform& srcToOutput,
                                          const DistortionModel& distortionModel,
                                          const DepthPerception& depthPerception,
                                          const QColor& bgColor) const {
  const CylindricalSurfaceDewarper dewarper(createDewarper(distortionModel, origToSrc, depthPerception.value()));

  // Model domain is a rectangle in output image coordinates that
  // will be mapped to our curved quadrilateral.
  const QRect modelDomain(distortionModel.modelDomain(dewarper, origToSrc * srcToOutput, m_outRect).toRect());
  if (modelDomain.isEmpty()) {
    GrayImage out(src.size());
    out.fill(0xff);  // white
    return out;
  }
  return RasterDewarper::dewarp(src, m_outRect.size(), dewarper, modelDomain, bgColor);
}

DistortionModel OutputGenerator::Processor::buildAutoDistortionModel(const GrayImage& warpedGrayOutput,
                                                                     const QTransform& toOriginal) const {
  DistortionModelBuilder modelBuilder(Vec2d(0, 1));

  TextLineTracer::trace(warpedGrayOutput, m_dpi, m_contentRectInWorkingCs, modelBuilder, m_status, m_dbg);
  modelBuilder.transform(toOriginal);

  TopBottomEdgeTracer::trace(m_inputGrayImage, modelBuilder.verticalBounds(), modelBuilder, m_status, m_dbg);

  DistortionModel distortionModel = modelBuilder.tryBuildModel(m_dbg, &m_inputGrayImage.toQImage());
  if (!distortionModel.isValid()) {
    setupTrivialDistortionModel(distortionModel);
  }

  BinaryImage bwImage(m_inputGrayImage, BinaryThreshold(64));

  QTransform transform = m_xform.preRotation().transform(bwImage.size());
  QTransform invTransform = transform.inverted();

  int degrees = m_xform.preRotation().toDegrees();
  bwImage = orthogonalRotation(bwImage, degrees);

  const std::vector<QPointF>& topPolyline0 = distortionModel.topCurve().polyline();
  const std::vector<QPointF>& bottomPolyline0 = distortionModel.bottomCurve().polyline();

  std::vector<QPointF> topPolyline;
  std::vector<QPointF> bottomPolyline;

  for (auto i : topPolyline0) {
    topPolyline.push_back(transform.map(i));
  }

  for (auto i : bottomPolyline0) {
    bottomPolyline.push_back(transform.map(i));
  }

  QString stAngle;

  float maxAngle = 2.75;

  if ((m_pageId.subPage() == PageId::SINGLE_PAGE) || (m_pageId.subPage() == PageId::LEFT_PAGE)) {
    float vertSkewAngleLeft = vertBorderSkewAngle(topPolyline.front(), bottomPolyline.front());

    stAngle.setNum(vertSkewAngleLeft);


    if (vertSkewAngleLeft > maxAngle) {
      auto topX = static_cast<float>(topPolyline.front().x());
      auto bottomX = static_cast<float>(bottomPolyline.front().x());

      if (topX < bottomX) {
        std::vector<QPointF> newBottomPolyline;

        QPointF pt(topX, bottomPolyline.front().y());

        newBottomPolyline.push_back(pt);

        for (auto i : bottomPolyline) {
          newBottomPolyline.push_back(invTransform.map(i));
        }

        distortionModel.setBottomCurve(Curve(newBottomPolyline));
      } else {
        std::vector<QPointF> newTopPolyline;

        QPointF pt(bottomX, topPolyline.front().y());

        newTopPolyline.push_back(pt);

        for (auto i : topPolyline) {
          newTopPolyline.push_back(invTransform.map(i));
        }

        distortionModel.setBottomCurve(Curve(newTopPolyline));
      }
    }
  } else {
    float vertSkewAngleRight = vertBorderSkewAngle(topPolyline.back(), bottomPolyline.back());

    stAngle.setNum(vertSkewAngleRight);


    if (vertSkewAngleRight > maxAngle) {
      auto topX = static_cast<float>(topPolyline.back().x());
      auto bottomX = static_cast<float>(bottomPolyline.back().x());

      if (topX > bottomX) {
        std::vector<QPointF> newBottomPolyline;

        QPointF pt(topX, bottomPolyline.back().y());

        for (auto i : bottomPolyline) {
          newBottomPolyline.push_back(invTransform.map(i));
        }

        newBottomPolyline.push_back(pt);

        distortionModel.setBottomCurve(Curve(newBottomPolyline));
      } else {
        std::vector<QPointF> newTopPolyline;

        QPointF pt(bottomX, topPolyline.back().y());

        for (auto i : topPolyline) {
          newTopPolyline.push_back(invTransform.map(i));
        }

        newTopPolyline.push_back(pt);

        distortionModel.setBottomCurve(Curve(newTopPolyline));
      }
    }
  }
  return distortionModel;
}

DistortionModel OutputGenerator::Processor::buildMarginalDistortionModel() const {
  BinaryImage bwImage(m_inputGrayImage, BinaryThreshold(64));

  QTransform transform = m_xform.preRotation().transform(bwImage.size());
  QTransform invTransform = transform.inverted();

  int degrees = m_xform.preRotation().toDegrees();
  bwImage = orthogonalRotation(bwImage, degrees);

  DistortionModel distortionModel;
  setupTrivialDistortionModel(distortionModel);

  int maxRedPoints = 5;
  XSpline topSpline;

  const std::vector<QPointF>& topPolyline = distortionModel.topCurve().polyline();

  const QLineF topLine(transform.map(topPolyline.front()), transform.map(topPolyline.back()));

  topSpline.appendControlPoint(topLine.p1(), 0);

  if ((m_pageId.subPage() == PageId::SINGLE_PAGE) || (m_pageId.subPage() == PageId::LEFT_PAGE)) {
    for (int i = 29 - maxRedPoints; i < 29; i++) {
      topSpline.appendControlPoint(topLine.pointAt((float) i / 29.0), 1);
    }
  } else {
    for (int i = 1; i <= maxRedPoints; i++) {
      topSpline.appendControlPoint(topLine.pointAt((float) i / 29.0), 1);
    }
  }

  topSpline.appendControlPoint(topLine.p2(), 0);

  for (int i = 0; i <= topSpline.numSegments(); i++) {
    movePointToTopMargin(bwImage, topSpline, i);
  }

  for (int i = 0; i <= topSpline.numSegments(); i++) {
    topSpline.moveControlPoint(i, invTransform.map(topSpline.controlPointPosition(i)));
  }

  distortionModel.setTopCurve(Curve(topSpline));


  XSpline bottomSpline;

  const std::vector<QPointF>& bottomPolyline = distortionModel.bottomCurve().polyline();

  const QLineF bottomLine(transform.map(bottomPolyline.front()), transform.map(bottomPolyline.back()));

  bottomSpline.appendControlPoint(bottomLine.p1(), 0);

  if ((m_pageId.subPage() == PageId::SINGLE_PAGE) || (m_pageId.subPage() == PageId::LEFT_PAGE)) {
    for (int i = 29 - maxRedPoints; i < 29; i++) {
      bottomSpline.appendControlPoint(topLine.pointAt((float) i / 29.0), 1);
    }
  } else {
    for (int i = 1; i <= maxRedPoints; i++) {
      bottomSpline.appendControlPoint(topLine.pointAt((float) i / 29.0), 1);
    }
  }

  bottomSpline.appendControlPoint(bottomLine.p2(), 0);

  for (int i = 0; i <= bottomSpline.numSegments(); i++) {
    movePointToBottomMargin(bwImage, bottomSpline, i);
  }

  for (int i = 0; i <= bottomSpline.numSegments(); i++) {
    bottomSpline.moveControlPoint(i, invTransform.map(bottomSpline.controlPointPosition(i)));
  }

  distortionModel.setBottomCurve(Curve(bottomSpline));

  if (!distortionModel.isValid()) {
    setupTrivialDistortionModel(distortionModel);
  }

  if (m_dbg) {
    QImage outImage(bwImage.toQImage().convertToFormat(QImage::Format_RGB32));
    for (int i = 0; i <= topSpline.numSegments(); i++) {
      drawPoint(outImage, topSpline.controlPointPosition(i));
    }
    for (int i = 0; i <= bottomSpline.numSegments(); i++) {
      drawPoint(outImage, bottomSpline.controlPointPosition(i));
    }
    m_dbg->add(outImage, "marginal dewarping");
  }
  return distortionModel;
}
}  // namespace output
