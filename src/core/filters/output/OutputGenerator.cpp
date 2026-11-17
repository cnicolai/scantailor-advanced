// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
#include "OutputGeneratorProcessor.h"
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
OutputGenerator::OutputGenerator(const ImageTransformation& xform, const QPolygonF& contentRectPhys)
    : m_xform(xform),
      m_outRect(xform.resultingRect().toRect()),
      m_contentRect(xform.transform().map(contentRectPhys).boundingRect().toRect()) {
  assert(m_outRect.topLeft() == QPoint(0, 0));

  if (!m_contentRect.isEmpty()) {
    // prevents a crash due to round error on transforming virtual coordinates to output image coordinates
    // when m_contentRect coordinates could exceed m_outRect ones by 1 px
    m_contentRect = m_contentRect.intersected(m_outRect);
    // Note that QRect::contains(<empty rect>) always returns false, so we don't use it here.
    assert(m_outRect.contains(m_contentRect.topLeft()) && m_outRect.contains(m_contentRect.bottomRight()));
  }
}

QSize OutputGenerator::outputImageSize() const {
  return m_outRect.size();
}

std::unique_ptr<OutputImage> OutputGenerator::process(const TaskStatus& status,
                                                      const FilterData& input,
                                                      ZoneSet& pictureZones,
                                                      const ZoneSet& fillZones,
                                                      DistortionModel& distortionModel,
                                                      const DepthPerception& depthPerception,
                                                      BinaryImage* autoPictureMask,
                                                      BinaryImage* specklesImage,
                                                      DebugImages* dbg,
                                                      const PageId& pageId,
                                                      const std::shared_ptr<Settings>& settings) const {
  return Processor(*this, pageId, settings, input, status, dbg)
      .process(pictureZones, fillZones, distortionModel, depthPerception, autoPictureMask, specklesImage);
}

OutputGenerator::Processor::Processor(const OutputGenerator& generator,
                                      const PageId& pageId,
                                      const std::shared_ptr<Settings>& settings,
                                      const FilterData& input,
                                      const TaskStatus& status,
                                      DebugImages* dbg)
    : m_xform(generator.m_xform),
      m_outRect(generator.m_outRect),
      m_contentRect(generator.m_contentRect),
      m_pageId(pageId),
      m_settings(settings),
      m_despeckleLevel(0),
      m_blank(false),
      m_blackOnWhite(true),
      m_colorOriginal(false),
      m_status(status),
      m_dbg(dbg) {
  initParams();
  calcAreas();
  if (!m_blank) {
    initFilterData(input);
  }
}

std::unique_ptr<OutputImage> OutputGenerator::Processor::process(ZoneSet& pictureZones,
                                                                 const ZoneSet& fillZones,
                                                                 DistortionModel& distortionModel,
                                                                 const DepthPerception& depthPerception,
                                                                 BinaryImage* autoPictureMask,
                                                                 BinaryImage* specklesImage) {
  std::unique_ptr<OutputImage> image
      = processImpl(pictureZones, fillZones, distortionModel, depthPerception, autoPictureMask, specklesImage);
  image->setDpm(m_dpi);
  return image;
}

void OutputGenerator::Processor::initParams() {
  const Params params = m_settings->getParams(m_pageId);
  m_dpi = params.outputDpi();
  m_colorParams = params.colorParams();
  m_splittingOptions = params.splittingOptions();
  m_pictureShapeOptions = params.pictureShapeOptions();
  m_dewarpingOptions = params.dewarpingOptions();
  m_despeckleLevel = params.despeckleLevel();

  m_outputProcessingParams = m_settings->getOutputProcessingParams(m_pageId);

  m_renderParams = RenderParams(m_colorParams, m_splittingOptions);
}

void OutputGenerator::Processor::calcAreas() {
  m_targetSize = m_outRect.size().expandedTo(QSize(1, 1));

  if (m_renderParams.fillOutsidePageBox()) {
    m_preCropArea = QRectF(m_outRect);
  } else if (m_renderParams.fillOffcut()) {
    m_preCropArea = m_xform.resultingPreCropArea();
  } else {
    const QPolygonF imageRectInOutputCs = m_xform.transform().map(m_xform.origRect());
    m_preCropArea = imageRectInOutputCs.intersected(QRectF(m_outRect));
  }

  m_croppedContentArea = m_preCropArea.intersected(QRectF(m_renderParams.fillMargins() ? m_contentRect : m_outRect));
  m_croppedContentRect = m_croppedContentArea.boundingRect().toRect();
  m_croppedContentArea = PolygonUtils::round(m_croppedContentArea.intersected(QRectF(m_croppedContentRect)));
  if (m_croppedContentRect.isEmpty()) {
    m_blank = true;
    return;
  }

  m_outCropArea = m_preCropArea.intersected(QRectF(m_outRect));

  // For various reasons, we need some whitespace around the content
  // area.  This is the number of pixels of such whitespace.
  const int contentMargin = m_dpi.vertical() * 20 / 300;
  // The content area (in output image coordinates) extended
  // with contentMargin.  Note that we prevent that extension
  // from reaching the neighboring page.
  // This is the area we are going to pass to estimateBackground().
  // estimateBackground() needs some margins around content, and
  // generally smaller margins are better, except when there is
  // some garbage that connects the content to the edge of the
  // image area.
  m_workingBoundingRect = m_preCropArea
                              .intersected(QRectF(m_croppedContentRect.adjusted(-contentMargin, -contentMargin,
                                                                                contentMargin, contentMargin)))
                              .boundingRect()
                              .toRect();
  m_contentRectInWorkingCs = m_croppedContentRect.translated(-m_workingBoundingRect.topLeft());
  m_contentAreaInWorkingCs = m_croppedContentArea.translated(-m_workingBoundingRect.topLeft());
  m_outCropAreaInWorkingCs = m_outCropArea.translated(-m_workingBoundingRect.topLeft());
  m_preCropAreaInOriginalCs = m_xform.transformBack().map(m_preCropArea);
  m_contentAreaInOriginalCs = m_xform.transformBack().map(m_croppedContentArea);
  m_outCropAreaInOriginalCs = m_xform.transformBack().map(m_outCropArea);
}

std::unique_ptr<OutputImage> OutputGenerator::Processor::buildEmptyImage() const {
  OutputImageBuilder imageBuilder;

  BinaryImage emptyImage(m_targetSize, WHITE);
  imageBuilder.setImage(emptyImage.toQImage());
  if (m_renderParams.splitOutput()) {
    imageBuilder.setForegroundMask(emptyImage);
    if (m_renderParams.originalBackground()) {
      imageBuilder.setBackgroundMask(emptyImage);
    }
  }
  return imageBuilder.build();
}

void OutputGenerator::Processor::updateBlackOnWhite(const FilterData& input) {
  ApplicationSettings& appSettings = ApplicationSettings::getInstance();
  Params params = m_settings->getParams(m_pageId);
  if ((appSettings.isBlackOnWhiteDetectionEnabled() && appSettings.isBlackOnWhiteDetectionOutputEnabled())
      && !m_settings->getOutputProcessingParams(m_pageId).isBlackOnWhiteSetManually()) {
    if (params.isBlackOnWhite() != input.isBlackOnWhite()) {
      params.setBlackOnWhite(input.isBlackOnWhite());
      m_settings->setParams(m_pageId, params);
    }
  }
  m_blackOnWhite = params.isBlackOnWhite();
}

void OutputGenerator::Processor::initFilterData(const FilterData& input) {
  updateBlackOnWhite(input);

  m_inputGrayImage = m_blackOnWhite ? input.grayImage() : input.grayImage().inverted();
  m_inputOrigImage = input.origImage();
  if (!m_blackOnWhite) {
    m_inputOrigImage.invertPixels();
  }
  m_colorOriginal = !m_inputOrigImage.allGray();
}

std::unique_ptr<OutputImage> OutputGenerator::Processor::processImpl(ZoneSet& pictureZones,
                                                                     const ZoneSet& fillZones,
                                                                     DistortionModel& distortionModel,
                                                                     const DepthPerception& depthPerception,
                                                                     BinaryImage* autoPictureMask,
                                                                     BinaryImage* specklesImage) {
  if (m_blank) {
    return buildEmptyImage();
  }

  m_outsideBackgroundColor = BackgroundColorCalculator::calcDominantBackgroundColor(
      m_colorOriginal ? m_inputOrigImage : m_inputGrayImage, m_outCropAreaInOriginalCs);

  if ((m_dewarpingOptions.dewarpingMode() == AUTO) || (m_dewarpingOptions.dewarpingMode() == MARGINAL)
      || ((m_dewarpingOptions.dewarpingMode() == MANUAL) && distortionModel.isValid())) {
    return processWithDewarping(pictureZones, fillZones, distortionModel, depthPerception, autoPictureMask,
                                specklesImage);
  } else {
    return processWithoutDewarping(pictureZones, fillZones, autoPictureMask, specklesImage);
  }
}
}  // namespace output
