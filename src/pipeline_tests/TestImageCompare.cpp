// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ImageCompare.h>
#include <Rng.h>
#include <Tolerances.h>

#include <QColor>
#include <QImage>
#include <QRgb>
#include <QString>
#include <boost/test/unit_test.hpp>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace test_support {
namespace tests {

namespace {

constexpr int kSide = 1000;
constexpr int kCell = 15;
constexpr int kColumns = 60;
constexpr int kLines = 20;

struct Glyph {
  int x;
  int y;
  int w;
  int h;
};

void setGrey(QImage& img, int x, int y, uint8_t value) {
  img.scanLine(y)[x] = value;
}

/// White paper and rows of block glyphs (filled rectangles, some with a notch), 8-connected components
/// separated by at least six pixels.  Rows from 600 down stay blank.  Greyscale input on purpose: the
/// comparator has to canonicalise it.
QImage makeTextLikePage(uint64_t seed, std::vector<Glyph>* glyphsOut) {
  QImage img(kSide, kSide, QImage::Format_Grayscale8);
  img.fill(255);
  Rng rng(seed);
  std::vector<Glyph> glyphs;
  for (int line = 0; line < kLines; ++line) {
    for (int col = 0; col < kColumns; ++col) {
      const Glyph g{20 + col * kCell, 40 + line * 28, int(5 + rng.below(5)), int(8 + rng.below(5))};
      const bool notch = rng.below(2) == 1;
      for (int y = g.y; y < g.y + g.h; ++y) {
        for (int x = g.x; x < g.x + g.w; ++x) {
          if (notch && x >= g.x + g.w - 2 && y < g.y + 2) {
            continue;
          }
          setGrey(img, x, y, 0);
        }
      }
      glyphs.push_back(g);
    }
  }
  if (glyphsOut) {
    *glyphsOut = glyphs;
  }
  return img;
}

QImage shiftedRight(const QImage& img, int dx) {
  QImage out(img.size(), img.format());
  out.fill(255);
  for (int y = 0; y < img.height(); ++y) {
    const uint8_t* src = img.constScanLine(y);
    uint8_t* dst = out.scanLine(y);
    for (int x = 0; x + dx < img.width(); ++x) {
      dst[x + dx] = src[x];
    }
  }
  return out;
}

const MetricResult* findMetric(const CompareResult& result, const QString& name) {
  for (const MetricResult& m : result.metrics) {
    if (m.metric == name) {
      return &m;
    }
  }
  return nullptr;
}

std::string describe(const CompareResult& result) {
  std::ostringstream out;
  for (const MetricResult& m : result.metrics) {
    out << m.metric.toStdString() << "=" << m.value << " (limit " << m.limit << ", " << (m.pass ? "pass" : "fail")
        << ") ";
  }
  return out.str();
}

void requireMetricPasses(const CompareResult& result, const char* name) {
  const MetricResult* m = findMetric(result, name);
  BOOST_REQUIRE_MESSAGE(m != nullptr, "metric " << name << " missing: " << describe(result));
  BOOST_CHECK_MESSAGE(m->pass, "metric " << name << " should pass: " << describe(result));
}

void requireMetricFails(const CompareResult& result, const char* name) {
  const MetricResult* m = findMetric(result, name);
  BOOST_REQUIRE_MESSAGE(m != nullptr, "metric " << name << " missing: " << describe(result));
  BOOST_CHECK_MESSAGE(!m->pass, "metric " << name << " should fail: " << describe(result));
}

/// Flat blocks with large steps between them, so that no shifted comparison can look better than the
/// unshifted one.
QImage makeColorPage() {
  QImage img(200, 200, QImage::Format_RGB32);
  img.fill(qRgb(30, 60, 90));
  for (int y = 20; y < 80; ++y) {
    for (int x = 20; x < 90; ++x) {
      img.setPixel(x, y, qRgb(220, 40, 40));
    }
  }
  for (int y = 100; y < 180; ++y) {
    for (int x = 60; x < 170; ++x) {
      img.setPixel(x, y, qRgb(40, 220, 40));
    }
  }
  for (int y = 30; y < 170; y += 10) {
    for (int x = 120; x < 190; ++x) {
      img.setPixel(x, y, qRgb(240, 240, 240));
    }
  }
  return img;
}

QImage plusLevels(const QImage& img, int delta) {
  QImage out = img.convertToFormat(QImage::Format_RGB32);
  for (int y = 0; y < out.height(); ++y) {
    QRgb* line = reinterpret_cast<QRgb*>(out.scanLine(y));
    for (int x = 0; x < out.width(); ++x) {
      line[x] = qRgb(qRed(line[x]) + delta, qGreen(line[x]) + delta, qBlue(line[x]) + delta);
    }
  }
  return out;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(ImageCompareTestSuite)

BOOST_AUTO_TEST_CASE(test_identical_bilevel_passes) {
  const QImage page = makeTextLikePage(11, nullptr);
  const CompareResult result = compareImages(page, page, ImageKind::BILEVEL);
  BOOST_CHECK_MESSAGE(result.pass, describe(result));
  requireMetricPasses(result, "edge_tolerant_xor_fraction");
  requireMetricPasses(result, "component_count_relative");
  requireMetricPasses(result, "shift");
  const MetricResult* shift = findMetric(result, "shift");
  BOOST_REQUIRE(shift != nullptr);
  BOOST_CHECK_EQUAL(shift->value, 0.0);
  BOOST_CHECK(result.diff.isNull());
}

BOOST_AUTO_TEST_CASE(test_one_pixel_shift_fails) {
  const QImage page = makeTextLikePage(11, nullptr);
  const CompareResult result = compareImages(page, shiftedRight(page, 1), ImageKind::BILEVEL);
  BOOST_CHECK_MESSAGE(!result.pass, describe(result));
  const MetricResult* shift = findMetric(result, "shift");
  BOOST_REQUIRE_MESSAGE(shift != nullptr, describe(result));
  BOOST_CHECK_EQUAL(shift->value, 1.0);
  BOOST_CHECK_EQUAL(shift->limit, 0.0);
  BOOST_CHECK(!shift->pass);
}

BOOST_AUTO_TEST_CASE(test_isolated_specks_fail_component_count) {
  const QImage page = makeTextLikePage(11, nullptr);
  QImage speckled = page;
  int placed = 0;
  // Lattice in the blank lower band: every speck has an all-paper 3x3 neighbourhood.
  for (int y = 650; y < 990 && placed < 300; y += 7) {
    for (int x = 10; x < 990 && placed < 300; x += 7) {
      setGrey(speckled, x, y, 0);
      ++placed;
    }
  }
  BOOST_REQUIRE_EQUAL(placed, 300);

  const CompareResult result = compareImages(page, speckled, ImageKind::BILEVEL);
  BOOST_CHECK_MESSAGE(!result.pass, describe(result));
  requireMetricFails(result, "component_count_relative");
}

BOOST_AUTO_TEST_CASE(test_edge_flips_are_tolerated) {
  std::vector<Glyph> glyphs;
  const QImage page = makeTextLikePage(11, &glyphs);
  QImage flipped = page;
  // Five paper pixels directly left of the first glyph of five lines: still attached to their glyph.
  for (int line = 0; line < 5; ++line) {
    const Glyph& g = glyphs[line * kColumns];
    setGrey(flipped, g.x - 1, g.y + g.h / 2, 0);
  }

  const CompareResult result = compareImages(page, flipped, ImageKind::BILEVEL);
  BOOST_CHECK_MESSAGE(result.pass, describe(result));
  const MetricResult* xorFraction = findMetric(result, "edge_tolerant_xor_fraction");
  BOOST_REQUIRE_MESSAGE(xorFraction != nullptr, describe(result));
  BOOST_CHECK_LT(xorFraction->value, tolerance::kBilevelEdgeTolerantXorFraction);
  requireMetricPasses(result, "component_count_relative");
  requireMetricPasses(result, "shift");
}

BOOST_AUTO_TEST_CASE(test_grey_level_offset_fails) {
  const QImage page = makeColorPage();
  const CompareResult result = compareImages(page, plusLevels(page, 10), ImageKind::COLOR);
  BOOST_CHECK_MESSAGE(!result.pass, describe(result));
  requireMetricFails(result, "mae");
  requireMetricFails(result, "max_abs_error");
}

BOOST_AUTO_TEST_CASE(test_single_level_change_passes) {
  const QImage page = makeColorPage();
  QImage changed = page;
  changed.setPixel(100, 100, qRgb(40, 221, 40));
  const CompareResult result = compareImages(page, changed, ImageKind::COLOR);
  BOOST_CHECK_MESSAGE(result.pass, describe(result));
  requireMetricPasses(result, "mae");
  requireMetricPasses(result, "max_abs_error");
  requireMetricPasses(result, "shift");
}

BOOST_AUTO_TEST_CASE(test_greyscale_vs_rgb_same_content_passes) {
  QImage grey(120, 80, QImage::Format_Indexed8);
  QVector<QRgb> ramp;
  for (int i = 0; i < 256; ++i) {
    ramp.push_back(qRgb(i, i, i));
  }
  grey.setColorTable(ramp);
  for (int y = 0; y < grey.height(); ++y) {
    for (int x = 0; x < grey.width(); ++x) {
      grey.scanLine(y)[x] = uint8_t(((x / 12) * 25 + (y / 10) * 7) % 256);
    }
  }
  const QImage rgb = grey.convertToFormat(QImage::Format_RGB32);

  const CompareResult forward = compareImages(grey, rgb, ImageKind::COLOR);
  BOOST_CHECK_MESSAGE(forward.pass, describe(forward));
  const CompareResult backward = compareImages(rgb, grey, ImageKind::COLOR);
  BOOST_CHECK_MESSAGE(backward.pass, describe(backward));
}

BOOST_AUTO_TEST_CASE(test_size_mismatch_fails) {
  const QImage a(100, 100, QImage::Format_RGB32);
  const QImage b(101, 100, QImage::Format_RGB32);
  for (ImageKind kind : {ImageKind::BILEVEL, ImageKind::COLOR}) {
    const CompareResult result = compareImages(a, b, kind);
    BOOST_CHECK(!result.pass);
    BOOST_REQUIRE_EQUAL(result.metrics.size(), 1u);
    BOOST_CHECK_EQUAL(result.metrics[0].metric.toStdString(), "size");
    BOOST_CHECK(!result.metrics[0].pass);
    BOOST_CHECK(!result.diff.isNull());
  }
}

BOOST_AUTO_TEST_CASE(test_failure_produces_diff_image) {
  std::vector<Glyph> glyphs;
  const QImage page = makeTextLikePage(11, &glyphs);
  const CompareResult result = compareImages(page, shiftedRight(page, 1), ImageKind::BILEVEL);
  BOOST_REQUIRE(!result.pass);
  BOOST_REQUIRE(!result.diff.isNull());
  BOOST_CHECK(result.diff.size() == page.size());
  BOOST_CHECK(result.diff.format() == QImage::Format_RGB32);

  const Glyph& g = glyphs[0];
  const int y = g.y + g.h - 1;  // below the notch, so the whole width is ink
  BOOST_CHECK_EQUAL(result.diff.pixel(g.x, y), qRgb(0, 0, 255));          // ink only in expected
  BOOST_CHECK_EQUAL(result.diff.pixel(g.x + g.w, y), qRgb(255, 0, 0));    // ink only in actual
  BOOST_CHECK_EQUAL(result.diff.pixel(g.x + 1, y), qRgb(64, 64, 64));     // ink in both
  BOOST_CHECK_EQUAL(result.diff.pixel(g.x - 3, y), qRgb(255, 255, 255));  // paper in both
}

BOOST_AUTO_TEST_CASE(test_color_failure_diff_marks_large_errors) {
  const QImage page = makeColorPage();
  const CompareResult result = compareImages(page, plusLevels(page, 10), ImageKind::COLOR);
  BOOST_REQUIRE(!result.pass);
  BOOST_REQUIRE(!result.diff.isNull());
  BOOST_CHECK(result.diff.size() == page.size());
  // 10 levels is above the max-error limit, so every pixel is marked red.
  BOOST_CHECK_EQUAL(result.diff.pixel(5, 5), qRgb(255, 0, 0));
}

BOOST_AUTO_TEST_CASE(test_canonical_forms) {
  QImage grey(8, 2, QImage::Format_Grayscale8);
  grey.fill(255);
  grey.scanLine(0)[3] = 0;
  grey.scanLine(1)[4] = 127;  // below 128: ink
  grey.scanLine(1)[5] = 128;  // paper

  const QImage bilevel = canonicalImage(grey, ImageKind::BILEVEL);
  BOOST_CHECK(bilevel.format() == QImage::Format_Mono);
  BOOST_CHECK(bilevel.pixelColor(3, 0) == QColor(Qt::black));
  BOOST_CHECK(bilevel.pixelColor(4, 1) == QColor(Qt::black));
  BOOST_CHECK(bilevel.pixelColor(5, 1) == QColor(Qt::white));
  BOOST_CHECK(bilevel.pixelColor(0, 0) == QColor(Qt::white));

  // The same picture stored in a mono image whose palette lists black first must canonicalise identically.
  QImage inverted(8, 2, QImage::Format_Mono);
  inverted.setColorTable({qRgb(0, 0, 0), qRgb(255, 255, 255)});
  inverted.fill(1);
  inverted.setPixel(3, 0, 0);
  inverted.setPixel(4, 1, 0);
  const QImage fromInverted = canonicalImage(inverted, ImageKind::BILEVEL);
  BOOST_CHECK(fromInverted.pixelColor(3, 0) == QColor(Qt::black));
  BOOST_CHECK(fromInverted.pixelColor(2, 0) == QColor(Qt::white));

  const QImage color = canonicalImage(grey, ImageKind::COLOR);
  BOOST_CHECK(color.format() == QImage::Format_RGB32);
  BOOST_CHECK_EQUAL(color.pixel(4, 1), qRgb(127, 127, 127));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace tests
}  // namespace test_support
