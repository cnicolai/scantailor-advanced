// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutputGenerator.h"

#include <DrawOver.h>
#include <RasterOp.h>
#include <imageproc/BackgroundColorCalculator.h>
#include <imageproc/WienerFilter.h>

#include <QSize>
#include <cmath>
#include <stdexcept>

#include "OutputGeneratorPixelOps.h"
#include "OutputGeneratorProcessor.h"
#include "OutputImageBuilder.h"

using namespace imageproc;
using namespace dewarping;

namespace output {
std::unique_ptr<OutputImage> OutputGenerator::Processor::processWithoutDewarping(ZoneSet& pictureZones,
                                                                                 const ZoneSet& fillZones,
                                                                                 BinaryImage* autoPictureMask,
                                                                                 BinaryImage* specklesImage) {
  QImage maybeNormalized, maybeSmoothed;
  BinaryImage bwContent, bwContentMaskOutput, bwContentOutput;
  OutputImageBuilder imageBuilder;

  maybeNormalized = transformToWorkingCs(m_renderParams.normalizeIllumination());
  if (m_dbg) {
    m_dbg->add(maybeNormalized, "maybeNormalized");
  }

  const ColorCommonOptions& colorCommonOptions = m_colorParams.colorCommonOptions();
  wienerColorFilterInPlace(maybeNormalized,
                           QSize(colorCommonOptions.wienerWindowSize(), colorCommonOptions.wienerWindowSize()),
                           colorCommonOptions.wienerCoef());

  if (m_renderParams.normalizeIllumination()) {
    m_outsideBackgroundColor
        = BackgroundColorCalculator::calcDominantBackgroundColor(maybeNormalized, m_outCropAreaInWorkingCs);
  }

  m_status.throwIfCancelled();

  if (m_renderParams.binaryOutput() || m_renderParams.mixedOutput()) {
    // BW mask begin
    if (!m_renderParams.needSavitzkyGolaySmoothing()) {
      maybeSmoothed = maybeNormalized;
    } else {
      maybeSmoothed = smoothToGrayscale(maybeNormalized, m_dpi);
      if (m_dbg) {
        m_dbg->add(maybeSmoothed, "smoothed");
      }
      m_status.throwIfCancelled();
    }

    bwContent = binarize(maybeSmoothed, m_contentAreaInWorkingCs);
    maybeSmoothed = QImage();  // Save memory.
    if (m_dbg) {
      m_dbg->add(bwContent, "binarized_and_cropped");
    }
    m_status.throwIfCancelled();

    if (m_renderParams.needMorphologicalSmoothing()) {
      morphologicalSmoothInPlace(bwContent);
      m_status.throwIfCancelled();
    }
    // BW mask end
  }

  // (BW only / Color segment) begin
  if (m_renderParams.binaryOutput()) {
    // don't destroy as it's needed for color segmentation
    if (!m_renderParams.needColorSegmentation()) {
      maybeNormalized = QImage();
    }

    BinaryImage dst(m_targetSize, WHITE);
    rasterOp<RopSrc>(dst, m_croppedContentRect, bwContent, m_contentRectInWorkingCs.topLeft());
    bwContent.release();  // Save memory.

    // It's important to keep despeckling the very last operation
    // affecting the binary part of the output. That's because
    // we will be reconstructing the input to this despeckling
    // operation from the final output file.
    maybeDespeckleInPlace(dst, m_outRect, m_outRect, m_despeckleLevel, specklesImage, m_dpi);

    if (!m_renderParams.needColorSegmentation()) {
      if (!m_blackOnWhite) {
        dst.invert();
      }

      applyFillZonesInPlace(dst, fillZones, m_xform.transform());

      imageBuilder.setImage(dst.toQImage());
    } else {
      QImage segmentedImage;
      {
        QImage colorImage(m_targetSize, maybeNormalized.format());
        if (maybeNormalized.format() == QImage::Format_Indexed8) {
          colorImage.setColorTable(maybeNormalized.colorTable());
        }
        colorImage.fill(Qt::white);
        drawOver(colorImage, m_croppedContentRect, maybeNormalized, m_contentRectInWorkingCs);
        maybeNormalized = QImage();

        segmentedImage = segmentImage(dst, colorImage);
        dst.release();
      }

      if (m_renderParams.posterize()) {
        segmentedImage = posterizeImage(segmentedImage, m_outsideBackgroundColor);
      }

      if (!m_blackOnWhite) {
        segmentedImage.invertPixels();
      }

      applyFillZonesInPlace(segmentedImage, fillZones, m_xform.transform(), false);
      if (m_dbg) {
        m_dbg->add(segmentedImage, "segmented_with_fill_zones");
      }
      m_status.throwIfCancelled();

      imageBuilder.setImage(segmentedImage);
    }
    return imageBuilder.build();
  }
  // (BW only / Color segment) end

  // Mixed begin
  if (m_renderParams.mixedOutput()) {
    BinaryImage bwMask(m_workingBoundingRect.size(), BLACK);
    processPictureZones(bwMask, pictureZones, GrayImage(maybeNormalized));
    if (m_dbg) {
      m_dbg->add(bwMask, "bwMask");
    }
    m_status.throwIfCancelled();

    if (autoPictureMask) {
      if (autoPictureMask->size() != m_targetSize) {
        BinaryImage(m_targetSize).swap(*autoPictureMask);
      }
      autoPictureMask->fill(BLACK);
      rasterOp<RopSrc>(*autoPictureMask, m_croppedContentRect, bwMask, m_contentRectInWorkingCs.topLeft());
    }
    m_status.throwIfCancelled();

    modifyBinarizationMask(bwMask, bwContent, m_workingBoundingRect, pictureZones);
    fillMarginsInPlace(bwMask, m_contentAreaInWorkingCs, BLACK);
    if (m_dbg) {
      m_dbg->add(bwMask, "bwMask with zones");
    }
    m_status.throwIfCancelled();

    if (m_renderParams.needBinarization()) {
      // It's important to keep despeckling the very last operation
      // affecting the binary part of the output. That's because
      // we will be reconstructing the input to this despeckling
      // operation from the final output file.
      maybeDespeckleInPlace(bwContent, m_workingBoundingRect, m_croppedContentRect, m_despeckleLevel, specklesImage,
                            m_dpi);

      if (!m_renderParams.normalizeIlluminationColor()) {
        m_outsideBackgroundColor = BackgroundColorCalculator::calcDominantBackgroundColor(
            m_colorOriginal ? m_inputOrigImage : m_inputGrayImage, m_outCropAreaInOriginalCs);
        maybeNormalized = transformToWorkingCs(false);
      }

      if (!m_renderParams.needColorSegmentation()) {
        if (m_renderParams.originalBackground()) {
          combineImages(maybeNormalized, bwContent);
        } else {
          combineImages(maybeNormalized, bwContent, bwMask);
        }
      } else {
        QImage segmentedImage = segmentImage(bwContent, maybeNormalized);
        if (m_renderParams.posterize()) {
          segmentedImage = posterizeImage(segmentedImage, m_outsideBackgroundColor);
        }

        if (m_renderParams.originalBackground()) {
          combineImages(maybeNormalized, segmentedImage);
        } else {
          combineImages(maybeNormalized, segmentedImage, bwMask);
        }
      }
      bwContent.release();  // Save memory.
      if (m_dbg) {
        m_dbg->add(maybeNormalized, "combined");
      }

      if (m_renderParams.originalBackground()) {
        reserveBlackAndWhite(maybeNormalized, bwContent.inverted());
      } else {
        reserveBlackAndWhite(maybeNormalized, bwMask.inverted());
      }
      m_status.throwIfCancelled();

      if (m_renderParams.originalBackground()) {
        bwContentOutput = BinaryImage(m_targetSize, WHITE);
        rasterOp<RopSrc>(bwContentOutput, m_croppedContentRect, bwContent, m_contentRectInWorkingCs.topLeft());
      }
    }

    bwContentMaskOutput = BinaryImage(m_targetSize, BLACK);
    rasterOp<RopSrc>(bwContentMaskOutput, m_croppedContentRect, bwMask, m_contentRectInWorkingCs.topLeft());
  }
  // Mixed end

  assert(!m_targetSize.isEmpty());
  QImage dst(m_targetSize, maybeNormalized.format());
  if (maybeNormalized.format() == QImage::Format_Indexed8) {
    dst.setColorTable(maybeNormalized.colorTable());
  }

  if (dst.isNull()) {
    // Both the constructor and setColorTable() above can leave the image null.
    throw std::bad_alloc();
  }

  if (m_renderParams.needBinarization() && !m_renderParams.originalBackground()) {
    switch (m_colorParams.colorCommonOptions().getFillingColor()) {
      case FILL_BLACK:
        m_outsideBackgroundColor = Qt::black;
        break;
      default:
        m_outsideBackgroundColor = Qt::white;
    }
  } else if (m_colorParams.colorCommonOptions().getFillingColor() == FILL_WHITE) {
    m_outsideBackgroundColor = m_blackOnWhite ? Qt::white : Qt::black;
  } else if (m_colorParams.colorCommonOptions().getFillingColor() == FILL_BLACK) {
    m_outsideBackgroundColor = m_blackOnWhite ? Qt::black : Qt::white;
  }

  fillMarginsInPlace(maybeNormalized, m_contentAreaInWorkingCs, m_outsideBackgroundColor);
  dst.fill(m_outsideBackgroundColor);

  drawOver(dst, m_croppedContentRect, maybeNormalized, m_contentRectInWorkingCs);
  maybeNormalized = QImage();

  if (!m_blackOnWhite) {
    dst.invertPixels();
  }

  if (m_renderParams.mixedOutput() && m_renderParams.needBinarization()) {
    applyFillZonesToMixedInPlace(dst, fillZones, m_xform.transform(), bwContentMaskOutput,
                                 !m_renderParams.needColorSegmentation());
  } else {
    applyFillZonesInPlace(dst, fillZones, m_xform.transform());
  }
  if (m_renderParams.originalBackground()) {
    applyFillZonesToMask(bwContentOutput, fillZones, m_xform.transform());
    rasterOp<RopAnd<RopSrc, RopDst>>(bwContentOutput, bwContentMaskOutput);
  }
  if (m_dbg) {
    m_dbg->add(dst, "fillZones");
  }
  m_status.throwIfCancelled();

  if (m_renderParams.posterize() && !m_renderParams.needBinarization()) {
    dst = posterizeImage(dst);
  }

  if (m_renderParams.splitOutput()) {
    imageBuilder.setForegroundType(getForegroundType());
    if (!m_renderParams.originalBackground()) {
      imageBuilder.setForegroundMask(bwContentMaskOutput);
    } else {
      imageBuilder.setForegroundMask(bwContentOutput);
      imageBuilder.setBackgroundMask(bwContentMaskOutput.inverted());
    }
  }
  return imageBuilder.setImage(dst).build();
}
}  // namespace output
