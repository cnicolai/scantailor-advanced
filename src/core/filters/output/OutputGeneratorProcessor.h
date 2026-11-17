// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_OUTPUTGENERATORPROCESSOR_H_
#define SCANTAILOR_OUTPUT_OUTPUTGENERATORPROCESSOR_H_

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
class OutputGenerator::Processor {
 public:
  Processor(const OutputGenerator& generator,
            const PageId& pageId,
            const std::shared_ptr<Settings>& settings,
            const FilterData& input,
            const TaskStatus& status,
            DebugImages* dbg);

  std::unique_ptr<OutputImage> process(ZoneSet& pictureZones,
                                       const ZoneSet& fillZones,
                                       DistortionModel& distortionModel,
                                       const DepthPerception& depthPerception,
                                       BinaryImage* autoPictureMask,
                                       BinaryImage* specklesImage);

 private:
  void initParams();

  void calcAreas();

  void updateBlackOnWhite(const FilterData& input);

  void initFilterData(const FilterData& input);

  std::unique_ptr<OutputImage> processImpl(ZoneSet& pictureZones,
                                           const ZoneSet& fillZones,
                                           DistortionModel& distortionModel,
                                           const DepthPerception& depthPerception,
                                           BinaryImage* autoPictureMask,
                                           BinaryImage* specklesImage);

  std::unique_ptr<OutputImage> processWithoutDewarping(ZoneSet& pictureZones,
                                                       const ZoneSet& fillZones,
                                                       BinaryImage* autoPictureMask,
                                                       BinaryImage* specklesImage);

  std::unique_ptr<OutputImage> processWithDewarping(ZoneSet& pictureZones,
                                                    const ZoneSet& fillZones,
                                                    DistortionModel& distortionModel,
                                                    const DepthPerception& depthPerception,
                                                    BinaryImage* autoPictureMask,
                                                    BinaryImage* specklesImage);

  std::unique_ptr<OutputImage> buildEmptyImage() const;

  double findSkew(const QImage& image) const;

  void setupTrivialDistortionModel(DistortionModel& distortionModel) const;

  static CylindricalSurfaceDewarper createDewarper(const DistortionModel& distortionModel,
                                                   const QTransform& distortionModelToTarget,
                                                   double depthPerception);

  QImage dewarp(const QTransform& origToSrc,
                const QImage& src,
                const QTransform& srcToOutput,
                const DistortionModel& distortionModel,
                const DepthPerception& depthPerception,
                const QColor& bgColor) const;

  GrayImage normalizeIlluminationGray(const QImage& input,
                                      const QPolygonF& areaToConsider,
                                      const QTransform& xform,
                                      const QRect& targetRect,
                                      GrayImage* background = nullptr) const;

  GrayImage detectPictures(const GrayImage& input300dpi) const;

  BinaryImage estimateBinarizationMask(const GrayImage& graySource,
                                       const QRect& sourceRect,
                                       const QRect& sourceSubRect) const;

  void modifyBinarizationMask(BinaryImage& bwMask,
                              BinaryImage& bwContent,
                              const QRect& maskRect,
                              const ZoneSet& zones) const;

  BinaryThreshold adjustThreshold(BinaryThreshold threshold) const;

  BinaryThreshold calcBinarizationThreshold(const QImage& image, const BinaryImage& mask) const;

  BinaryThreshold calcBinarizationThreshold(const QImage& image,
                                            const QPolygonF& cropArea,
                                            const BinaryImage* mask = nullptr) const;

  void morphologicalSmoothInPlace(BinaryImage& binImg) const;

  BinaryImage binarize(const QImage& image) const;

  BinaryImage binarize(const QImage& image, const BinaryImage& mask) const;

  BinaryImage binarize(const QImage& image, const QPolygonF& cropArea, const BinaryImage* mask = nullptr) const;

  void maybeDespeckleInPlace(BinaryImage& image,
                             const QRect& imageRect,
                             const QRect& maskRect,
                             double level,
                             BinaryImage* specklesImg,
                             const Dpi& dpi) const;

  QImage segmentImage(const BinaryImage& image, const QImage& colorImage) const;

  QImage posterizeImage(const QImage& image, const QColor& backgroundColor = Qt::white) const;

  ForegroundType getForegroundType() const;

  void processPictureZones(BinaryImage& mask, ZoneSet& pictureZones, const GrayImage& image);

  QImage transformToWorkingCs(bool normalize) const;

  DistortionModel buildAutoDistortionModel(const GrayImage& warpedGrayOutput, const QTransform& toOriginal) const;

  DistortionModel buildMarginalDistortionModel() const;

  const ImageTransformation& m_xform;
  const QRect& m_outRect;
  const QRect& m_contentRect;

  const PageId m_pageId;
  const std::shared_ptr<Settings> m_settings;

  Dpi m_dpi;
  ColorParams m_colorParams;
  SplittingOptions m_splittingOptions;
  PictureShapeOptions m_pictureShapeOptions;
  DewarpingOptions m_dewarpingOptions;
  double m_despeckleLevel;
  OutputProcessingParams m_outputProcessingParams;

  RenderParams m_renderParams;

  QPolygonF m_preCropArea;
  QPolygonF m_croppedContentArea;
  QRect m_croppedContentRect;
  QPolygonF m_outCropArea;
  QSize m_targetSize;
  QRect m_workingBoundingRect;
  QRect m_contentRectInWorkingCs;
  QPolygonF m_contentAreaInWorkingCs;
  QPolygonF m_outCropAreaInWorkingCs;
  QPolygonF m_preCropAreaInOriginalCs;
  QPolygonF m_contentAreaInOriginalCs;
  QPolygonF m_outCropAreaInOriginalCs;
  bool m_blank;

  bool m_blackOnWhite;
  QImage m_inputOrigImage;
  GrayImage m_inputGrayImage;
  bool m_colorOriginal;

  const TaskStatus& m_status;
  DebugImages* const m_dbg;

  QColor m_outsideBackgroundColor;
};
}  // namespace output

#endif  // SCANTAILOR_OUTPUT_OUTPUTGENERATORPROCESSOR_H_
