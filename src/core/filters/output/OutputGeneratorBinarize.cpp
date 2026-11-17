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
#include "OutputGeneratorPixelOps.h"
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
namespace {
struct RaiseAboveBackground {
  static uint8_t transform(uint8_t src, uint8_t dst) {
    // src: orig
    // dst: background (dst >= src)
    if (dst - src < 1) {
      return 0xff;
    }
    const unsigned orig = src;
    const unsigned background = dst;
    return static_cast<uint8_t>((orig * 255 + background / 2) / background);
  }
};

struct CombineInverted {
  static uint8_t transform(uint8_t src, uint8_t dst) {
    const unsigned dilated = dst;
    const unsigned eroded = src;
    const unsigned res = 255 - (255 - dilated) * eroded / 255;
    return static_cast<uint8_t>(res);
  }
};

void BinaryImageXOR(BinaryImage& image, const BinaryImage& bwMask, const BWColor color) {
  uint32_t* imageLine = image.data();
  const int imageStride = image.wordsPerLine();
  const uint32_t* bwMaskLine = bwMask.data();
  const int bwMaskStride = bwMask.wordsPerLine();
  const int width = image.width();
  const int height = image.height();
  const uint32_t msb = uint32_t(1) << 31;

  if (color == BLACK) {
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        if ((imageLine[x >> 5] & (msb >> (x & 31))) != (bwMaskLine[x >> 5] & (msb >> (x & 31)))) {
          imageLine[x >> 5] |= (msb >> (x & 31));
        } else {
          imageLine[x >> 5] &= ~(msb >> (x & 31));
        }
      }
      imageLine += imageStride;
      bwMaskLine += bwMaskStride;
    }
  } else {
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        if ((imageLine[x >> 5] & (msb >> (x & 31))) != (bwMaskLine[x >> 5] & (msb >> (x & 31)))) {
          imageLine[x >> 5] &= ~(msb >> (x & 31));
        } else {
          imageLine[x >> 5] |= (msb >> (x & 31));
        }
      }
      imageLine += imageStride;
      bwMaskLine += bwMaskStride;
    }
  }
}

void removeAutoPictureZones(ZoneSet& pictureZones) {
  for (auto it = pictureZones.begin(); it != pictureZones.end();) {
    const Zone& zone = *it;
    if (zone.properties().locateOrDefault<ZoneCategoryProperty>()->zoneCategory() == ZoneCategoryProperty::AUTO) {
      it = pictureZones.erase(it);
    } else {
      ++it;
    }
  }
}

Zone createPictureZoneFromPoly(const QPolygonF& polygon) {
  PropertySet propertySet;
  propertySet.locateOrCreate<output::PictureLayerProperty>()->setLayer(output::PictureLayerProperty::ZONEPAINTER2);
  propertySet.locateOrCreate<output::ZoneCategoryProperty>()->setZoneCategory(ZoneCategoryProperty::AUTO);
  return Zone(SerializableSpline(polygon), propertySet);
}

const int MultiplyDeBruijnBitPosition[32] = {0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
                                             31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9};

const int MultiplyDeBruijnBitPosition2[32] = {0, 9,  1,  10, 13, 21, 2,  29, 11, 14, 16, 18, 22, 25, 3, 30,
                                              8, 12, 20, 28, 15, 17, 24, 7,  19, 27, 23, 6,  26, 5,  4, 31};

/**
 * aka "Count the consecutive zero bits (trailing) on the right with multiply and lookup"
 * from Bit Twiddling Hacks By Sean Eron Anderson
 *
 * https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
 */
inline int countConsecutiveZeroBitsTrailing(uint32_t v) {
  return MultiplyDeBruijnBitPosition[((uint32_t) ((v & -signed(v)) * 0x077CB531U)) >> 27];
}

/**
 * aka "Find the log base 2 of an N-bit integer in O(lg(N)) operations with multiply and lookup"
 * from Bit Twiddling Hacks By Sean Eron Anderson
 *
 * https://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
 */
inline int findPositionOfTheHighestBitSet(uint32_t v) {
  v |= v >> 1;  // first round down to one less than a power of 2
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  return MultiplyDeBruijnBitPosition2[(uint32_t) (v * 0x07C4ACDDU) >> 27];
}

std::vector<QRect> findRectAreas(const BinaryImage& mask, BWColor contentColor, int sensitivity) {
  if (mask.isNull()) {
    return {};
  }

  std::vector<QRect> areas;

  const int w = mask.width();
  const int h = mask.height();
  const int wpl = mask.wordsPerLine();
  const int lastWordIdx = (w - 1) >> 5;
  const int lastWordBits = w - (lastWordIdx << 5);
  const int lastWordUnusedBits = 32 - lastWordBits;
  const uint32_t lastWordMask = ~uint32_t(0) << lastWordUnusedBits;
  const uint32_t modifier = (contentColor == WHITE) ? ~uint32_t(0) : 0;
  const uint32_t* const data = mask.data();

  const uint32_t* line = data;
  // create list of filled continuous blocks on each line
  for (int y = 0; y < h; ++y, line += wpl) {
    QRect area;
    area.setTop(y);
    area.setBottom(y);
    bool areaFound = false;
    for (int i = 0; i <= lastWordIdx; ++i) {
      uint32_t word = line[i] ^ modifier;
      if (i == lastWordIdx) {
        // The last (possibly incomplete) word.
        word &= lastWordMask;
      }
      if (word) {
        if (!areaFound) {
          area.setLeft((i << 5) + 31 - findPositionOfTheHighestBitSet(~line[i]));
          areaFound = true;
        }
        area.setRight(((i + 1) << 5) - 1);
      } else {
        if (areaFound) {
          uint32_t v = line[i - 1];
          if (v) {
            area.setRight(area.right() - countConsecutiveZeroBitsTrailing(~v));
          }
          areas.emplace_back(area);
          areaFound = false;
        }
      }
    }
    if (areaFound) {
      uint32_t v = line[lastWordIdx];
      if (v) {
        area.setRight(area.right() - countConsecutiveZeroBitsTrailing(~v));
      }
      areas.emplace_back(area);
    }
  }

  // join adjacent blocks of areas
  bool join = true;
  int overlap = 16;
  while (join) {
    join = false;
    std::vector<QRect> tmp;
    for (QRect area : areas) {
      // take an area and try to join with something in tmp
      QRect enlArea(area.adjusted(-overlap, -overlap, overlap, overlap));
      bool intersected = false;
      std::vector<QRect> tmp2;
      for (QRect ta : tmp) {
        QRect enlTA(ta.adjusted(-overlap, -overlap, overlap, overlap));
        if (enlArea.intersects(enlTA)) {
          intersected = true;
          join = true;
          tmp2.push_back(area.united(ta));
        } else {
          tmp2.push_back(ta);
        }
      }
      if (!intersected) {
        tmp2.push_back(area);
      }
      tmp = tmp2;
    }
    areas = tmp;
  }

  const auto percent = (float) (sensitivity / 100.);
  if (percent < 1.) {
    for (QRect& area : areas) {
      int wordWidth = area.width() >> 5;

      int left = area.left();
      int leftWord = left >> 5;
      int right = area.x() + area.width();
      int rightWord = right >> 5;
      int top = area.top();
      int bottom = area.bottom();

      const uint32_t* pdata = mask.data();

      const auto criterium = (int) (area.width() * percent);
      const auto criteriumWord = (int) (wordWidth * percent);

      // cut the dirty upper lines
      for (int y = top; y < bottom; y++) {
        line = pdata + wpl * y;

        int mword = 0;

        for (int k = leftWord; k < rightWord; k++) {
          if (!line[k]) {
            mword++;  // count the totally white words
          }
        }

        if (mword > criteriumWord) {
          area.setTop(y);
          break;
        }
      }

      // cut the dirty bottom lines
      for (int y = bottom; y > top; y--) {
        line = pdata + wpl * y;

        int mword = 0;

        for (int k = leftWord; k < rightWord; k++) {
          if (!line[k]) {
            mword++;
          }
        }

        if (mword > criteriumWord) {
          area.setBottom(y);
          break;
        }
      }

      for (int x = left; x < right; x++) {
        int mword = 0;

        for (int y = top; y < bottom; y++) {
          if (WHITE == mask.getPixel(x, y)) {
            mword++;
          }
        }

        if (mword > criterium) {
          area.setLeft(x);
          break;
        }
      }

      for (int x = right; x > left; x--) {
        int mword = 0;

        for (int y = top; y < bottom; y++) {
          if (WHITE == mask.getPixel(x, y)) {
            mword++;
          }
        }

        if (mword > criterium) {
          area.setRight(x);
          break;
        }
      }

      area = area.intersected(mask.rect());
    }
  }
  return areas;
}

void hitMissReplaceAllDirections(BinaryImage& img,
                                 const char* const pattern,
                                 const int patternWidth,
                                 const int patternHeight) {
  hitMissReplaceInPlace(img, WHITE, pattern, patternWidth, patternHeight);

  std::vector<char> patternData(static_cast<unsigned long long int>(patternWidth * patternHeight), ' ');
  char* const newPattern = &patternData[0];

  // Rotate 90 degrees clockwise.
  const char* p = pattern;
  int newWidth = patternHeight;
  int newHeight = patternWidth;
  for (int y = 0; y < patternHeight; ++y) {
    for (int x = 0; x < patternWidth; ++x, ++p) {
      const int newX = patternHeight - 1 - y;
      const int newY = x;
      newPattern[newY * newWidth + newX] = *p;
    }
  }
  hitMissReplaceInPlace(img, WHITE, newPattern, newWidth, newHeight);

  // Rotate upside down.
  p = pattern;
  newWidth = patternWidth;
  newHeight = patternHeight;
  for (int y = 0; y < patternHeight; ++y) {
    for (int x = 0; x < patternWidth; ++x, ++p) {
      const int newX = patternWidth - 1 - x;
      const int newY = patternHeight - 1 - y;
      newPattern[newY * newWidth + newX] = *p;
    }
  }
  hitMissReplaceInPlace(img, WHITE, newPattern, newWidth, newHeight);
  // Rotate 90 degrees counter-clockwise.
  p = pattern;
  newWidth = patternHeight;
  newHeight = patternWidth;
  for (int y = 0; y < patternHeight; ++y) {
    for (int x = 0; x < patternWidth; ++x, ++p) {
      const int newX = y;
      const int newY = patternWidth - 1 - x;
      newPattern[newY * newWidth + newX] = *p;
    }
  }
  hitMissReplaceInPlace(img, WHITE, newPattern, newWidth, newHeight);
}

QSize calcLocalWindowSize(const Dpi& dpi) {
  const QSizeF sizeMm(3, 30);
  const QSizeF sizeInch(sizeMm * constants::MM2INCH);
  const QSizeF sizePixelsF(dpi.horizontal() * sizeInch.width(), dpi.vertical() * sizeInch.height());
  QSize sizePixels(sizePixelsF.toSize());

  if (sizePixels.width() < 3) {
    sizePixels.setWidth(3);
  }
  if (sizePixels.height() < 3) {
    sizePixels.setHeight(3);
  }
  return sizePixels;
}

QSize from300dpi(const QSize& size, const Dpi& targetDpi) {
  const double hscale = targetDpi.horizontal() / 300.0;
  const double vscale = targetDpi.vertical() / 300.0;
  const int width = qRound(size.width() * hscale);
  const int height = qRound(size.height() * vscale);
  return QSize(std::max(1, width), std::max(1, height));
}

QSize to300dpi(const QSize& size, const Dpi& sourceDpi) {
  const double hscale = 300.0 / sourceDpi.horizontal();
  const double vscale = 300.0 / sourceDpi.vertical();
  const int width = qRound(size.width() * hscale);
  const int height = qRound(size.height() * vscale);
  return QSize(std::max(1, width), std::max(1, height));
}
}  // namespace

GrayImage OutputGenerator::Processor::normalizeIlluminationGray(const QImage& input,
                                                                const QPolygonF& areaToConsider,
                                                                const QTransform& xform,
                                                                const QRect& targetRect,
                                                                GrayImage* background) const {
  GrayImage toBeNormalized = transformToGray(input, xform, targetRect, OutsidePixels::assumeWeakNearest());
  if (m_dbg) {
    m_dbg->add(toBeNormalized, "toBeNormalized");
  }

  m_status.throwIfCancelled();

  QPolygonF transformedConsiderationArea = xform.map(areaToConsider);
  transformedConsiderationArea.translate(-targetRect.topLeft());

  const PolynomialSurface bgPs = estimateBackground(toBeNormalized, transformedConsiderationArea, m_status, m_dbg);
  m_status.throwIfCancelled();

  GrayImage bgImg(bgPs.render(toBeNormalized.size()));
  if (m_dbg) {
    m_dbg->add(bgImg, "background");
  }
  if (background) {
    *background = bgImg;
  }
  m_status.throwIfCancelled();

  grayRasterOp<RaiseAboveBackground>(bgImg, toBeNormalized);
  if (m_dbg) {
    m_dbg->add(bgImg, "normalized_illumination");
  }
  m_status.throwIfCancelled();
  return bgImg;
}

BinaryImage OutputGenerator::Processor::estimateBinarizationMask(const GrayImage& graySource,
                                                                 const QRect& sourceRect,
                                                                 const QRect& sourceSubRect) const {
  assert(sourceRect.contains(sourceSubRect));

  // If we need to strip some of the margins from a grayscale
  // image, we may actually do it without copying anything.
  // We are going to construct a QImage from existing data.
  // That image won't own that data, but graySource is not
  // going anywhere, so it's fine.

  GrayImage trimmedImage;

  if (sourceRect == sourceSubRect) {
    trimmedImage = graySource;  // Shallow copy.
  } else {
    // Sub-rectangle in input image coordinates.
    QRect relativeSubrect(sourceSubRect);
    relativeSubrect.moveTopLeft(sourceSubRect.topLeft() - sourceRect.topLeft());

    const int stride = graySource.stride();
    const int offset = relativeSubrect.top() * stride + relativeSubrect.left();

    trimmedImage = GrayImage(QImage(graySource.data() + offset, relativeSubrect.width(), relativeSubrect.height(),
                                    stride, QImage::Format_Indexed8));
  }

  m_status.throwIfCancelled();

  const QSize downscaledSize(to300dpi(trimmedImage.size(), m_dpi));

  // A 300dpi version of trimmedImage.
  GrayImage downscaledInput(scaleToGray(trimmedImage, downscaledSize));
  trimmedImage = GrayImage();  // Save memory.
  m_status.throwIfCancelled();

  // Light areas indicate pictures.
  GrayImage pictureAreas(detectPictures(downscaledInput));
  downscaledInput = GrayImage();  // Save memory.
  m_status.throwIfCancelled();

  const BinaryThreshold threshold(48);
  // Scale back to original size.
  pictureAreas = scaleToGray(pictureAreas, sourceSubRect.size());
  return BinaryImage(pictureAreas, threshold);
}

void OutputGenerator::Processor::modifyBinarizationMask(BinaryImage& bwMask,
                                                        BinaryImage& bwContent,
                                                        const QRect& maskRect,
                                                        const ZoneSet& zones) const {
  QTransform xform = m_xform.transform();
  xform *= QTransform().translate(-maskRect.x(), -maskRect.y());
  BinaryImage bwContentBG(bwContent);

  using PLP = PictureLayerProperty;

  // Pass 1: ZONEERASER1
  for (const Zone& zone : zones) {
    if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::ZONEERASER1) {
      const QPolygonF poly(zone.spline().toPolygon());
      PolygonRasterizer::fill(bwMask, BLACK, xform.map(poly), Qt::WindingFill);
    }
  }

  // Pass 2: ZONEFG
  BinaryImageXOR(bwMask, bwContent, WHITE);
  for (const Zone& zone : zones) {
    if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::ZONEFG) {
      const QPolygonF poly(zone.spline().toPolygon());
      PolygonRasterizer::fill(bwMask, WHITE, xform.map(poly), Qt::WindingFill);
      PolygonRasterizer::fill(bwContentBG, WHITE, xform.map(poly), Qt::WindingFill);
    }
  }
  BinaryImageXOR(bwMask, bwContent, WHITE);

  // Pass 3: ZONEBG
  BinaryImageXOR(bwMask, bwContentBG, BLACK);
  for (const Zone& zone : zones) {
    if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::ZONEBG) {
      const QPolygonF poly(zone.spline().toPolygon());
      PolygonRasterizer::fill(bwMask, WHITE, xform.map(poly), Qt::WindingFill);
    }
  }
  BinaryImageXOR(bwMask, bwContentBG, BLACK);

  // Pass 4: ZONEPAINTER2
  for (const Zone& zone : zones) {
    if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::ZONEPAINTER2) {
      const QPolygonF poly(zone.spline().toPolygon());
      PolygonRasterizer::fill(bwMask, WHITE, xform.map(poly), Qt::WindingFill);
    }
  }

  // Pass 5: ZONEERASER3
  for (const Zone& zone : zones) {
    if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::ZONEERASER3) {
      const QPolygonF poly(zone.spline().toPolygon());
      PolygonRasterizer::fill(bwMask, BLACK, xform.map(poly), Qt::WindingFill);
    }
  }

  BinaryImageXOR(bwContent, bwMask, WHITE);
}

GrayImage OutputGenerator::Processor::detectPictures(const GrayImage& input300dpi) const {
  // We stretch the range of gray levels to cover the whole
  // range of [0, 255].  We do it because we want text
  // and background to be equally far from the center
  // of the whole range.  Otherwise text printed with a big
  // font will be considered a picture.
  GrayImage stretched(stretchGrayRange(input300dpi, 0.01, 0.01));
  if (m_dbg) {
    m_dbg->add(stretched, "stretched");
  }

  m_status.throwIfCancelled();

  GrayImage eroded(erodeGray(stretched, QSize(3, 3), 0x00));
  if (m_dbg) {
    m_dbg->add(eroded, "eroded");
  }

  m_status.throwIfCancelled();

  GrayImage dilated(dilateGray(stretched, QSize(3, 3), 0xff));
  if (m_dbg) {
    m_dbg->add(dilated, "dilated");
  }

  stretched = GrayImage();  // Save memory.
  m_status.throwIfCancelled();

  grayRasterOp<CombineInverted>(dilated, eroded);
  GrayImage grayGradient(dilated);
  dilated = GrayImage();
  eroded = GrayImage();
  if (m_dbg) {
    m_dbg->add(grayGradient, "grayGradient");
  }

  m_status.throwIfCancelled();

  GrayImage marker(erodeGray(grayGradient, QSize(35, 35), 0x00));
  if (m_dbg) {
    m_dbg->add(marker, "marker");
  }

  m_status.throwIfCancelled();

  seedFillGrayInPlace(marker, grayGradient, CONN8);
  GrayImage reconstructed(marker);
  marker = GrayImage();
  if (m_dbg) {
    m_dbg->add(reconstructed, "reconstructed");
  }

  m_status.throwIfCancelled();

  grayRasterOp<GRopInvert<GRopSrc>>(reconstructed, reconstructed);
  if (m_dbg) {
    m_dbg->add(reconstructed, "reconstructed_inverted");
  }

  m_status.throwIfCancelled();

  GrayImage holesFilled(createFramedImage(reconstructed.size()));
  seedFillGrayInPlace(holesFilled, reconstructed, CONN8);
  reconstructed = GrayImage();
  if (m_dbg) {
    m_dbg->add(holesFilled, "holesFilled");
  }

  if (m_pictureShapeOptions.isHigherSearchSensitivity()) {
    GrayImage stretched2(stretchGrayRange(holesFilled, 5.0, 0.01));
    if (m_dbg) {
      m_dbg->add(stretched2, "stretched2");
    }
    return stretched2;
  }
  return holesFilled;
}

BinaryThreshold OutputGenerator::Processor::adjustThreshold(BinaryThreshold threshold) const {
  const int adjusted = threshold + m_colorParams.blackWhiteOptions().thresholdAdjustment();

  // Hard-bounding threshold values is necessary for example
  // if all the content went into the picture mask.
  return BinaryThreshold(qBound(30, adjusted, 225));
}

BinaryThreshold OutputGenerator::Processor::calcBinarizationThreshold(const QImage& image,
                                                                      const BinaryImage& mask) const {
  GrayscaleHistogram hist(image, mask);
  return adjustThreshold(BinaryThreshold::otsuThreshold(hist));
}

BinaryThreshold OutputGenerator::Processor::calcBinarizationThreshold(const QImage& image,
                                                                      const QPolygonF& cropArea,
                                                                      const BinaryImage* mask) const {
  QPainterPath path;
  path.addPolygon(cropArea);

  if (path.contains(image.rect())) {
    return adjustThreshold(BinaryThreshold::otsuThreshold(image));
  } else {
    BinaryImage modifiedMask(image.size(), BLACK);
    PolygonRasterizer::fillExcept(modifiedMask, WHITE, cropArea, Qt::WindingFill);
    modifiedMask = erodeBrick(modifiedMask, QSize(3, 3), WHITE);

    if (mask) {
      rasterOp<RopAnd<RopSrc, RopDst>>(modifiedMask, *mask);
    }
    return calcBinarizationThreshold(image, modifiedMask);
  }
}

void OutputGenerator::Processor::morphologicalSmoothInPlace(BinaryImage& binImg) const {
  // When removing black noise, remove small ones first.

  {
    const char pattern[]
        = "XXX"
          " - "
          "   ";
    hitMissReplaceAllDirections(binImg, pattern, 3, 3);
  }

  m_status.throwIfCancelled();

  {
    const char pattern[]
        = "X ?"
          "X  "
          "X- "
          "X- "
          "X  "
          "X ?";
    hitMissReplaceAllDirections(binImg, pattern, 3, 6);
  }

  m_status.throwIfCancelled();

  {
    const char pattern[]
        = "X ?"
          "X ?"
          "X  "
          "X- "
          "X- "
          "X- "
          "X  "
          "X ?"
          "X ?";
    hitMissReplaceAllDirections(binImg, pattern, 3, 9);
  }

  m_status.throwIfCancelled();

  {
    const char pattern[]
        = "XX?"
          "XX?"
          "XX "
          "X+ "
          "X+ "
          "X+ "
          "XX "
          "XX?"
          "XX?";
    hitMissReplaceAllDirections(binImg, pattern, 3, 9);
  }

  m_status.throwIfCancelled();

  {
    const char pattern[]
        = "XX?"
          "XX "
          "X+ "
          "X+ "
          "XX "
          "XX?";
    hitMissReplaceAllDirections(binImg, pattern, 3, 6);
  }

  m_status.throwIfCancelled();

  {
    const char pattern[]
        = "   "
          "X+X"
          "XXX";
    hitMissReplaceAllDirections(binImg, pattern, 3, 3);
  }

  if (m_dbg) {
    m_dbg->add(binImg, "edges_smoothed");
  }
}

BinaryImage OutputGenerator::Processor::binarize(const QImage& image) const {
  if ((image.format() == QImage::Format_Mono) || (image.format() == QImage::Format_MonoLSB)) {
    return BinaryImage(image);
  }

  const BlackWhiteOptions& blackWhiteOptions = m_colorParams.blackWhiteOptions();
  const BinarizationMethod binarizationMethod = blackWhiteOptions.getBinarizationMethod();

  BinaryImage binarized;
  switch (binarizationMethod) {
    case T_OTSU: {
      GrayscaleHistogram hist(image);
      const BinaryThreshold bwThresh(BinaryThreshold::otsuThreshold(hist));

      binarized = BinaryImage(image, adjustThreshold(bwThresh));
      break;
    }
    case T_SAUVOLA: {
      const double thresholdDelta = blackWhiteOptions.thresholdAdjustment();
      const QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(), blackWhiteOptions.getWindowSize());
      const double thresholdCoef = blackWhiteOptions.getSauvolaCoef();

      binarized = binarizeSauvola(image, windowsSize, thresholdCoef, thresholdDelta);
      break;
    }
    case T_WOLF: {
      const double thresholdDelta = blackWhiteOptions.thresholdAdjustment();
      const QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(), blackWhiteOptions.getWindowSize());
      const auto lowerBound = (unsigned char) blackWhiteOptions.getWolfLowerBound();
      const auto upperBound = (unsigned char) blackWhiteOptions.getWolfUpperBound();
      const double thresholdCoef = blackWhiteOptions.getWolfCoef();

      binarized = binarizeWolf(image, windowsSize, lowerBound, upperBound, thresholdCoef, thresholdDelta);
      break;
    }
    case T_FOX: {
      const double thresholdDelta = blackWhiteOptions.thresholdAdjustment();
      const QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(), blackWhiteOptions.getWindowSize());
      const auto lowerBound = (unsigned char) blackWhiteOptions.getWolfLowerBound();
      const auto upperBound = (unsigned char) blackWhiteOptions.getWolfUpperBound();
      const double thresholdCoef = blackWhiteOptions.getWolfCoef();

      binarized = binarizeFox(image, windowsSize, lowerBound, upperBound, thresholdCoef, thresholdDelta);
      break;
    }
    case T_WINDOW: {
      const double thresholdDelta = blackWhiteOptions.thresholdAdjustment();
      const QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(), blackWhiteOptions.getWindowSize());
      const auto lowerBound = (unsigned char) blackWhiteOptions.getWolfLowerBound();
      const auto upperBound = (unsigned char) blackWhiteOptions.getWolfUpperBound();
      const double thresholdCoef = blackWhiteOptions.getWolfCoef();

      binarized = binarizeWindow(image, windowsSize, lowerBound, upperBound, thresholdCoef, thresholdDelta);
      break;
    }
    case T_BRADLEY: {
      const double thresholdDelta = blackWhiteOptions.thresholdAdjustment();
      const QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(), blackWhiteOptions.getWindowSize());
      const double thresholdCoef = blackWhiteOptions.getSauvolaCoef();

      binarized = binarizeBradley(image, windowsSize, thresholdCoef, thresholdDelta);
      break;
    }
    case T_GRAD: {
      const double thresholdDelta = blackWhiteOptions.thresholdAdjustment();
      const QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(), blackWhiteOptions.getWindowSize());
      const auto lowerBound = (unsigned char) blackWhiteOptions.getWolfLowerBound();
      const auto upperBound = (unsigned char) blackWhiteOptions.getWolfUpperBound();
      const double thresholdCoef = blackWhiteOptions.getWolfCoef();

      binarized = binarizeGrad(image, windowsSize, lowerBound, upperBound, thresholdCoef, thresholdDelta);
      break;
    }
    case T_EDGEPLUS: {
      const double thresholdDelta = blackWhiteOptions.thresholdAdjustment();
      const QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(), blackWhiteOptions.getWindowSize());
      const double thresholdCoef = blackWhiteOptions.getSauvolaCoef();

      binarized = binarizeEdgeDiv(image, windowsSize, thresholdCoef, 0.0, thresholdDelta);
      break;
    }
    case T_BLURDIV: {
      const double thresholdDelta = blackWhiteOptions.thresholdAdjustment();
      const QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(), blackWhiteOptions.getWindowSize());
      const double thresholdCoef = blackWhiteOptions.getSauvolaCoef();

      binarized = binarizeEdgeDiv(image, windowsSize, 0.0, thresholdCoef, thresholdDelta);
      break;
    }
    case T_EDGEDIV: {
      const double thresholdDelta = blackWhiteOptions.thresholdAdjustment();
      const QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(), blackWhiteOptions.getWindowSize());
      const double thresholdCoef = blackWhiteOptions.getSauvolaCoef();

      binarized = binarizeEdgeDiv(image, windowsSize, thresholdCoef, thresholdCoef, thresholdDelta);
      break;
    }
  }
  return binarized;
}

BinaryImage OutputGenerator::Processor::binarize(const QImage& image, const BinaryImage& mask) const {
  BinaryImage binarized = binarize(image);

  rasterOp<RopAnd<RopSrc, RopDst>>(binarized, mask);
  return binarized;
}

BinaryImage OutputGenerator::Processor::binarize(const QImage& image,
                                                 const QPolygonF& cropArea,
                                                 const BinaryImage* mask) const {
  QPainterPath path;
  path.addPolygon(cropArea);

  if (path.contains(image.rect()) && !mask) {
    return binarize(image);
  } else {
    BinaryImage modifiedMask(image.size(), BLACK);
    PolygonRasterizer::fillExcept(modifiedMask, WHITE, cropArea, Qt::WindingFill);
    modifiedMask = erodeBrick(modifiedMask, QSize(3, 3), WHITE);

    if (mask) {
      rasterOp<RopAnd<RopSrc, RopDst>>(modifiedMask, *mask);
    }
    return binarize(image, modifiedMask);
  }
}

/**
 * \brief Remove small connected components that are considered to be garbage.
 *
 * Both the size and the distance to other components are taken into account.
 *
 * \param[in,out] image The image to despeckle.
 * \param imageRect The rectangle corresponding to \p image in the same
 *        coordinate system where m_contentRect and m_cropRect are defined.
 * \param maskRect The area within the image to consider.  Defined not
 *        relative to \p image, but in the same coordinate system where
 *        m_contentRect and m_cropRect are defined.  This only affects
 *        \p specklesImg, if provided.
 * \param level Despeckling aggressiveness.
 * \param specklesImg If provided, the removed black speckles will be written
 *        there.  The speckles image is always considered to correspond
 *        to m_cropRect, so it will have the size of m_cropRect.size().
 *        Only the area within \p maskRect will be copied to \p specklesImg.
 *        The rest will be filled with white.
 * \param dpi The DPI of the input image.  See the note below.
 * \param status Task status.
 * \param dbg An optional sink for debugging images.
 *
 * \note This function only works effectively when the DPI is symmetric,
 * that is, its horizontal and vertical components are equal.
 */
void OutputGenerator::Processor::maybeDespeckleInPlace(BinaryImage& image,
                                                       const QRect& imageRect,
                                                       const QRect& maskRect,
                                                       double level,
                                                       BinaryImage* specklesImg,
                                                       const Dpi& dpi) const {
  const QRect srcRect(maskRect.translated(-imageRect.topLeft()));
  const QRect dstRect(maskRect);

  if (specklesImg) {
    BinaryImage(m_outRect.size(), WHITE).swap(*specklesImg);
    if (!maskRect.isEmpty()) {
      rasterOp<RopSrc>(*specklesImg, dstRect, image, srcRect.topLeft());
    }
  }

  if (level != 0) {
    Despeckle::despeckleInPlace(image, dpi, level, m_status, m_dbg);

    if (m_dbg) {
      m_dbg->add(image, "despeckled");
    }
  }

  if (specklesImg) {
    if (!maskRect.isEmpty()) {
      rasterOp<RopSubtract<RopDst, RopSrc>>(*specklesImg, dstRect, image, srcRect.topLeft());
    }
  }

  m_status.throwIfCancelled();
}

double OutputGenerator::Processor::findSkew(const QImage& image) const {
  if (m_dewarpingOptions.needPostDeskew()
      && ((m_dewarpingOptions.dewarpingMode() == MARGINAL) || (m_dewarpingOptions.dewarpingMode() == MANUAL))) {
    const BinaryImage bwImage(image, BinaryThreshold::otsuThreshold(GrayscaleHistogram(image)));
    const Skew skew = SkewFinder().findSkew(bwImage);
    if ((skew.angle() != .0) && (skew.confidence() >= Skew::GOOD_CONFIDENCE)) {
      return skew.angle();
    }
  }
  return .0;
}

QImage OutputGenerator::Processor::segmentImage(const BinaryImage& image, const QImage& colorImage) const {
  const BlackWhiteOptions::ColorSegmenterOptions& segmenterOptions
      = m_colorParams.blackWhiteOptions().getColorSegmenterOptions();
  ColorSegmenter segmenter(m_dpi, segmenterOptions.getNoiseReduction(), segmenterOptions.getRedThresholdAdjustment(),
                           segmenterOptions.getGreenThresholdAdjustment(),
                           segmenterOptions.getBlueThresholdAdjustment());

  QImage segmented;
  {
    if (m_colorOriginal) {
      segmented = segmenter.segment(image, colorImage);
    } else {
      segmented = segmenter.segment(image, GrayImage(colorImage));
    }
  }

  if (m_dbg) {
    m_dbg->add(segmented, "segmented");
  }
  m_status.throwIfCancelled();
  return segmented;
}

QImage OutputGenerator::Processor::posterizeImage(const QImage& image, const QColor& backgroundColor) const {
  const ColorCommonOptions::PosterizationOptions& posterizationOptions
      = m_colorParams.colorCommonOptions().getPosterizationOptions();
  Posterizer posterizer(posterizationOptions.getLevel(), posterizationOptions.isNormalizationEnabled(),
                        posterizationOptions.isForceBlackAndWhite(), 0, qRound(backgroundColor.lightnessF() * 255));

  QImage posterized = posterizer.posterize(image);

  if (m_dbg) {
    m_dbg->add(posterized, "posterized");
  }
  m_status.throwIfCancelled();
  return posterized;
}

void OutputGenerator::Processor::processPictureZones(BinaryImage& mask, ZoneSet& pictureZones, const GrayImage& image) {
  if ((m_pictureShapeOptions.getPictureShape() != RECTANGULAR_SHAPE) || !m_outputProcessingParams.isAutoZonesFound()) {
    if (m_pictureShapeOptions.getPictureShape() != OFF_SHAPE) {
      mask = estimateBinarizationMask(image, m_workingBoundingRect, m_workingBoundingRect);
    }

    removeAutoPictureZones(pictureZones);
    m_settings->setPictureZones(m_pageId, pictureZones);
    m_outputProcessingParams.setAutoZonesFound(false);
    m_settings->setOutputProcessingParams(m_pageId, m_outputProcessingParams);
  }
  if ((m_pictureShapeOptions.getPictureShape() == RECTANGULAR_SHAPE) && !m_outputProcessingParams.isAutoZonesFound()) {
    std::vector<QRect> areas = findRectAreas(mask, WHITE, m_pictureShapeOptions.getSensitivity());

    const QTransform fromWorkingCs = [this]() {
      QTransform toWorkingCs = m_xform.transform();
      toWorkingCs *= QTransform().translate(-m_workingBoundingRect.x(), -m_workingBoundingRect.y());
      return toWorkingCs.inverted();
    }();

    for (const QRect& area : areas) {
      pictureZones.add(createPictureZoneFromPoly(fromWorkingCs.map(QRectF(area))));
    }
    m_settings->setPictureZones(m_pageId, pictureZones);
    m_outputProcessingParams.setAutoZonesFound(true);
    m_settings->setOutputProcessingParams(m_pageId, m_outputProcessingParams);

    mask.fill(BLACK);
  }
}

ForegroundType OutputGenerator::Processor::getForegroundType() const {
  if (m_renderParams.needBinarization()) {
    if (m_renderParams.needColorSegmentation()) {
      return ForegroundType::INDEXED;
    } else {
      return ForegroundType::BINARY;
    }
  } else {
    return ForegroundType::COLOR;
  }
}

QImage OutputGenerator::Processor::transformToWorkingCs(bool normalize) const {
  QImage dst;
  if (normalize) {
    dst = normalizeIlluminationGray(m_inputGrayImage, m_preCropAreaInOriginalCs, m_xform.transform(),
                                    m_workingBoundingRect);
    if (m_colorOriginal) {
      assert(dst.format() == QImage::Format_Indexed8);
      QImage colorImg = transform(m_inputOrigImage, m_xform.transform(), m_workingBoundingRect,
                                  OutsidePixels::assumeColor(m_outsideBackgroundColor));
      adjustBrightnessGrayscale(colorImg, dst);
      dst = colorImg;
    }
  } else {
    if (!m_colorOriginal) {
      dst = transformToGray(m_inputGrayImage, m_xform.transform(), m_workingBoundingRect,
                            OutsidePixels::assumeColor(m_outsideBackgroundColor));
    } else {
      dst = transform(m_inputOrigImage, m_xform.transform(), m_workingBoundingRect,
                      OutsidePixels::assumeColor(m_outsideBackgroundColor));
    }
  }
  m_status.throwIfCancelled();
  return dst;
}
}  // namespace output
