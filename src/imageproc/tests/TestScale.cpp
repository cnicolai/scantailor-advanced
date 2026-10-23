// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <GrayImage.h>
#include <Rng.h>
#include <Scale.h>

#include <QImage>
#include <QSize>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(ScaleTestSuite)

BOOST_AUTO_TEST_CASE(test_null_image) {
  const GrayImage nullImg;
  BOOST_CHECK(scaleToGray(nullImg, QSize(1, 1)).isNull());
}

static bool fuzzyCompare(const QImage& img1, const QImage& img2) {
  BOOST_REQUIRE(img1.size() == img2.size());

  const int width = img1.width();
  const int height = img1.height();
  const uint8_t* line1 = img1.bits();
  const uint8_t* line2 = img2.bits();
  const int line1Bpl = img1.bytesPerLine();
  const int line2Bpl = img2.bytesPerLine();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (std::abs(int(line1[x]) - int(line2[x])) > 1) {
        return false;
      }
    }
    line1 += line1Bpl;
    line2 += line2Bpl;
  }
  return true;
}

// Exact area-average reference: every output pixel is the mean of the input pixels its footprint covers, each
// weighted by the covered area, computed in double precision and rounded half up.  QImage::scaled() is not a
// usable reference any more: since Qt 6 its smooth scaling is about one level darker on average and up to 28 levels
// off on an 80x80 downscale (records/unit-tests.md, "D-17 TestScale investigation").
static QImage areaAverageReference(const QImage& src, const QSize& dstSize) {
  QImage dst(dstSize, QImage::Format_Grayscale8);
  const double scaleX = double(src.width()) / dstSize.width();
  const double scaleY = double(src.height()) / dstSize.height();
  for (int dy = 0; dy < dstSize.height(); ++dy) {
    uint8_t* const dstLine = dst.scanLine(dy);
    const double top = dy * scaleY;
    const double bottom = (dy + 1) * scaleY;
    for (int dx = 0; dx < dstSize.width(); ++dx) {
      const double left = dx * scaleX;
      const double right = (dx + 1) * scaleX;
      double weightedSum = 0.0;
      double area = 0.0;
      for (int sy = static_cast<int>(top); sy < static_cast<int>(std::ceil(bottom)); ++sy) {
        const double heightShare = std::min<double>(sy + 1, bottom) - std::max<double>(sy, top);
        const uint8_t* const srcLine = src.constScanLine(sy);
        for (int sx = static_cast<int>(left); sx < static_cast<int>(std::ceil(right)); ++sx) {
          const double share = (std::min<double>(sx + 1, right) - std::max<double>(sx, left)) * heightShare;
          weightedSum += share * srcLine[sx];
          area += share;
        }
      }
      dstLine[dx] = static_cast<uint8_t>(std::floor(weightedSum / area + 0.5));
    }
  }
  return dst;
}

static bool checkScale(const GrayImage& img, const QSize& newSize) {
  const GrayImage scaled1(scaleToGray(img, newSize));
  const GrayImage scaled2(areaAverageReference(img.toQImage(), newSize));
  return fuzzyCompare(scaled1, scaled2);
}

BOOST_AUTO_TEST_CASE(test_random_image) {
  GrayImage img(QSize(100, 100));
  test_support::Rng rng(7);
  uint8_t* line = img.data();
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      line[x] = static_cast<uint8_t>(rng.below(256));
    }
    line += img.stride();
  }

  // Unfortunately scaleToGray() and QImage::scaled()
  // produce too different results when upscaling.

  BOOST_CHECK(checkScale(img, QSize(50, 50)));
  // BOOST_CHECK(checkScale(img, QSize(200, 200)));
  BOOST_CHECK(checkScale(img, QSize(80, 80)));
  // BOOST_CHECK(checkScale(img, QSize(140, 140)));
  // BOOST_CHECK(checkScale(img, QSize(55, 145)));
  // BOOST_CHECK(checkScale(img, QSize(145, 55)));
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc