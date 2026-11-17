// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <PageGenerator.h>

#include <QByteArray>
#include <QImage>
#include <QLineF>
#include <QPointF>
#include <QRect>
#include <QRgb>
#include <QString>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <exception>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

using test_support::GeneratedPage;
using test_support::PageKind;
using test_support::PageSpec;

namespace {

const PageKind kKinds[] = {PageKind::FLAT, PageKind::SPREAD, PageKind::MIXED, PageKind::CURVED};
const int kDpis[] = {300, 600};

std::string kindName(const PageKind kind) {
  switch (kind) {
    case PageKind::FLAT:
      return "FLAT";
    case PageKind::SPREAD:
      return "SPREAD";
    case PageKind::MIXED:
      return "MIXED";
    case PageKind::CURVED:
      return "CURVED";
  }
  return "?";
}

double skewFor(const PageKind kind) {
  switch (kind) {
    case PageKind::FLAT:
      return 1.5;
    case PageKind::SPREAD:
      return 1.0;
    default:
      return 0.5;
  }
}

PageSpec makeSpec(const PageKind kind, const int dpi, const uint64_t seed = 1) {
  PageSpec spec;
  spec.kind = kind;
  spec.dpi = dpi;
  spec.seed = seed;
  spec.skewDeg = skewFor(kind);
  return spec;
}

std::string label(const PageSpec& spec) {
  return kindName(spec.kind) + " " + std::to_string(spec.dpi) + " dpi";
}

QSize basePageSize(const PageKind kind) {
  return kind == PageKind::SPREAD ? QSize(1654, 1181) : QSize(1000, 1150);
}

// Base units of 1/300 in scaled to the dpi, rounded to whole pixels like the generator (exact at 300 and 600).
int scaled(const int base, const int dpi) {
  return (base * dpi + 150) / 300;
}

// Rendering is slow in a Debug build, so pages that tests only read are rendered once per spec.  The determinism
// cases render their own copies on purpose.
const GeneratedPage& cachedPage(const PageSpec& spec) {
  static std::map<std::tuple<int, int, uint64_t, double>, GeneratedPage> cache;
  const auto key = std::make_tuple(static_cast<int>(spec.kind), spec.dpi, spec.seed, spec.skewDeg);
  auto it = cache.find(key);
  if (it == cache.end()) {
    it = cache.emplace(key, test_support::generatePage(spec)).first;
  }
  return it->second;
}

double xAtY(const QLineF& line, const double y) {
  return line.p1().x() + (y - line.p1().y()) * (line.p2().x() - line.p1().x()) / (line.p2().y() - line.p1().y());
}

bool near(const double a, const double b, const double tolerance) {
  return std::fabs(a - b) <= tolerance;
}

const int kSweepDpis[] = {75, 90, 100, 150, 200, 225, 450};

// Whether the generator must render a page for the kind at a sweep dpi (see test_dpi_sweep_gives_valid_or_null_page).
bool sweepExpectsPage(const PageKind kind, const int dpi) {
  if (kind == PageKind::FLAT) {
    return dpi != 90;
  }
  return dpi == 75 || dpi == 200;
}

// Unlike QRect::contains, an empty rectangle (a speck of side 0 at 75 dpi) counts as inside when it sits on the page.
bool insidePage(const QRect& r, const QSize& page) {
  return r.left() >= 0 && r.top() >= 0 && r.right() < page.width() && r.bottom() < page.height();
}

// An exception from the generator becomes a check failure naming kind and dpi.
const GeneratedPage* sweepPage(const PageSpec& spec) {
  try {
    return &cachedPage(spec);
  } catch (const std::exception& e) {
    BOOST_ERROR(label(spec) << ": generatePage threw: " << e.what());
    return nullptr;
  }
}

QImage sweepMask(const PageSpec& spec, const int targetDpi) {
  try {
    return test_support::renderInkMask(spec, targetDpi, 0.0);
  } catch (const std::exception& e) {
    BOOST_ERROR(label(spec) << " at target dpi " << targetDpi << ": renderInkMask threw: " << e.what());
    return QImage();
  }
}

}  // namespace

BOOST_AUTO_TEST_SUITE(GeneratorDeterminismTestSuite)

BOOST_AUTO_TEST_CASE(test_same_seed_same_pixels) {
  for (const PageKind kind : kKinds) {
    for (const int dpi : kDpis) {
      const PageSpec spec = makeSpec(kind, dpi);
      const GeneratedPage first = test_support::generatePage(spec);
      const GeneratedPage second = test_support::generatePage(spec);
      BOOST_REQUIRE_MESSAGE(!first.image.isNull() && !second.image.isNull(), label(spec) << ": null image");
      const QByteArray a = test_support::fingerprint(first.image);
      const QByteArray b = test_support::fingerprint(second.image);
      BOOST_CHECK_MESSAGE(a.size() == 64, label(spec) << ": fingerprint is not 64 hex characters: " << a.toStdString());
      BOOST_CHECK_MESSAGE(a == b, label(spec) << ": same seed gave different fingerprints");
      BOOST_CHECK_MESSAGE(first.image == second.image, label(spec) << ": same seed gave different pixels");
    }
  }
}

BOOST_AUTO_TEST_CASE(test_different_seed_different_pixels) {
  for (const PageKind kind : kKinds) {
    for (const int dpi : kDpis) {
      const PageSpec spec = makeSpec(kind, dpi, 1);
      const PageSpec other = makeSpec(kind, dpi, 2);
      const QByteArray a = test_support::fingerprint(cachedPage(spec).image);
      const QByteArray b = test_support::fingerprint(cachedPage(other).image);
      BOOST_CHECK_MESSAGE(!a.isEmpty() && a != b, label(spec) << ": seed 2 gave the same fingerprint as seed 1");
    }
  }
}

BOOST_AUTO_TEST_CASE(test_format_size_and_dpi) {
  for (const PageKind kind : kKinds) {
    for (const int dpi : kDpis) {
      const PageSpec spec = makeSpec(kind, dpi);
      const GeneratedPage& page = cachedPage(spec);
      const QSize expected(scaled(basePageSize(kind).width(), dpi), scaled(basePageSize(kind).height(), dpi));
      const int dotsPerMeter = static_cast<int>(dpi / 0.0254 + 0.5);
      BOOST_CHECK_MESSAGE(page.image.format() == QImage::Format_RGB32, label(spec) << ": not Format_RGB32");
      BOOST_CHECK_MESSAGE(page.image.size() == expected,
                          label(spec) << ": size " << page.image.width() << "x" << page.image.height());
      BOOST_CHECK_EQUAL(page.image.dotsPerMeterX(), dotsPerMeter);
      BOOST_CHECK_EQUAL(page.image.dotsPerMeterY(), dotsPerMeter);
      BOOST_CHECK_MESSAGE(page.truth.pageSize == page.image.size(), label(spec) << ": truth.pageSize differs");
      BOOST_CHECK_EQUAL(page.truth.dpi, dpi);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_spread_truth_consistent) {
  for (const int dpi : kDpis) {
    const PageSpec spec = makeSpec(PageKind::SPREAD, dpi);
    const GeneratedPage& page = cachedPage(spec);
    const test_support::GroundTruth& t = page.truth;
    const QRect pageRect(QPoint(0, 0), page.image.size());

    BOOST_CHECK_MESSAGE(pageRect.contains(t.leftBlock) && pageRect.contains(t.rightBlock),
                        label(spec) << ": a block is outside the page");
    BOOST_CHECK_MESSAGE(!t.leftBlock.isEmpty() && !t.rightBlock.isEmpty(), label(spec) << ": empty block");
    BOOST_CHECK_MESSAGE(t.leftBlock.right() < t.rightBlock.left(), label(spec) << ": blocks overlap horizontally");
    BOOST_CHECK_MESSAGE(t.textBlock == t.leftBlock.united(t.rightBlock), label(spec) << ": textBlock is not the union");
    BOOST_CHECK_EQUAL(t.lineCount, 15);
    BOOST_CHECK_EQUAL(t.lineTops.size(), static_cast<size_t>(t.lineCount));

    // Block frames are 640 base px wide; the ink is a little narrower.
    BOOST_CHECK_MESSAGE(t.leftBlock.width() <= scaled(640, dpi) && t.rightBlock.width() <= scaled(640, dpi),
                        label(spec) << ": a block is wider than its 640 px frame");
    BOOST_CHECK_MESSAGE(t.leftBlock.height() <= scaled(860, dpi) && t.rightBlock.height() <= scaled(860, dpi),
                        label(spec) << ": a block is taller than its 860 px frame");

    const double midY = page.image.height() / 2.0;
    const double gutterX = xAtY(t.gutterCentre, midY);
    BOOST_CHECK_MESSAGE(gutterX > t.leftBlock.right() && gutterX < t.rightBlock.left(),
                        label(spec) << ": gutter at x = " << gutterX << " is not between the blocks");
    BOOST_CHECK_MESSAGE(t.gutterCentre.p1().y() < t.gutterCentre.p2().y(), label(spec) << ": gutter line points up");
    BOOST_CHECK_MESSAGE(near(t.gutterHalfWidthPx, 113.5 * dpi / 300.0, 1e-9),
                        label(spec) << ": half width " << t.gutterHalfWidthPx);

    // Without skew the gutter is the vertical line through the middle of the 227 px gap.
    PageSpec flat = spec;
    flat.skewDeg = 0.0;
    const test_support::GroundTruth u = cachedPage(flat).truth;
    const double expectedX = 826.5 * dpi / 300.0;
    BOOST_CHECK_MESSAGE(near(u.gutterCentre.p1().x(), expectedX, 1e-6) && near(u.gutterCentre.p2().x(), expectedX, 1e-6)
                            && near(u.gutterCentre.p1().y(), 0.0, 1e-6)
                            && near(u.gutterCentre.p2().y(), page.image.height(), 1e-6),
                        label(spec) << ": unskewed gutter line is not the page-high vertical at " << expectedX);
  }
}

BOOST_AUTO_TEST_CASE(test_mixed_truth_consistent) {
  for (const int dpi : kDpis) {
    const PageSpec spec = makeSpec(PageKind::MIXED, dpi);
    const GeneratedPage& page = cachedPage(spec);
    const test_support::GroundTruth& t = page.truth;
    const QRect pageRect(QPoint(0, 0), page.image.size());
    const int margin = scaled(20, dpi);

    BOOST_CHECK_MESSAGE(t.pictureRect == QRect(scaled(580, dpi), scaled(300, dpi), scaled(320, dpi), scaled(400, dpi)),
                        label(spec) << ": picture rectangle is not 580..900 x 300..700 base px");
    BOOST_CHECK_MESSAGE(pageRect.contains(t.pictureRect), label(spec) << ": picture outside the page");
    BOOST_CHECK_MESSAGE(!t.textBlock.isEmpty() && pageRect.contains(t.textBlock), label(spec) << ": bad text block");
    BOOST_CHECK_MESSAGE(!t.textBlock.intersects(t.pictureRect), label(spec) << ": picture overlaps the text");
    BOOST_CHECK_EQUAL(t.lineCount, 15);
    BOOST_CHECK_EQUAL(t.lineTops.size(), static_cast<size_t>(t.lineCount));

    BOOST_REQUIRE_EQUAL(t.specks.size(), 40u);
    const QRect farFromText = t.textBlock.adjusted(-margin, -margin, margin, margin);
    const QRect farFromPicture = t.pictureRect.adjusted(-margin, -margin, margin, margin);
    const QRect insideEdges = pageRect.adjusted(margin, margin, -margin, -margin);
    for (size_t i = 0; i < t.specks.size(); ++i) {
      const QRect& speck = t.specks[i];
      BOOST_CHECK_MESSAGE(
          speck.width() == speck.height() && speck.width() >= scaled(1, dpi) && speck.width() <= scaled(3, dpi),
          label(spec) << ": speck " << i << " is not a square of 1 to 3 base px");
      BOOST_CHECK_MESSAGE(insideEdges.contains(speck), label(spec)
                                                           << ": speck " << i << " is within 20 px of the edge");
      BOOST_CHECK_MESSAGE(!speck.intersects(farFromText), label(spec)
                                                              << ": speck " << i << " is within 20 px of the text");
      BOOST_CHECK_MESSAGE(!speck.intersects(farFromPicture),
                          label(spec) << ": speck " << i << " is within 20 px of the picture");
      for (size_t j = i + 1; j < t.specks.size(); ++j) {
        BOOST_CHECK_MESSAGE(!speck.intersects(t.specks[j]), label(spec)
                                                                << ": specks " << i << " and " << j << " overlap");
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(test_curved_truth_consistent) {
  for (const int dpi : kDpis) {
    const PageSpec spec = makeSpec(PageKind::CURVED, dpi);
    const GeneratedPage& page = cachedPage(spec);
    const test_support::GroundTruth& t = page.truth;
    const QRect pageRect(QPoint(0, 0), page.image.size());

    BOOST_CHECK_MESSAGE(near(t.curlAmplitudePx, 25.0 * dpi / 300.0, 1e-9), label(spec)
                                                                               << ": amplitude " << t.curlAmplitudePx);
    BOOST_CHECK_EQUAL(t.lineCount, 14);
    BOOST_REQUIRE_EQUAL(t.lineTops.size(), static_cast<size_t>(t.lineCount));
    BOOST_CHECK_MESSAGE(!t.textBlock.isEmpty() && pageRect.contains(t.textBlock), label(spec) << ": bad text block");
    BOOST_CHECK_MESSAGE(t.textBlock.width() <= scaled(700, dpi) && t.textBlock.height() <= scaled(800, dpi),
                        label(spec) << ": the text block is larger than its 700 x 800 frame");

    // The top line is lifted by the full amplitude at the spine edge and not at all at the far edge.
    PageSpec straight = spec;
    straight.skewDeg = 0.0;
    const double top = t.lineTops.front();
    const QPointF atSpine = test_support::mapFlatToImage(straight, QPointF(t.textBlock.right(), top));
    const QPointF atFar = test_support::mapFlatToImage(straight, QPointF(t.textBlock.left(), top));
    BOOST_CHECK_MESSAGE(atSpine.y() < top - 0.8 * t.curlAmplitudePx && atSpine.y() > top - 1.0001 * t.curlAmplitudePx,
                        label(spec) << ": top line at the spine edge moves to y = " << atSpine.y() << " from " << top);
    BOOST_CHECK_MESSAGE(near(atFar.y(), top, 0.5), label(spec)
                                                       << ": top line at the far edge moves to y = " << atFar.y());
    BOOST_CHECK_MESSAGE(atSpine.x() > t.textBlock.right(), label(spec) << ": no compression towards the spine");
  }
}

BOOST_AUTO_TEST_CASE(test_ink_mask_inside_text_block) {
  for (const PageKind kind : kKinds) {
    const PageSpec spec = makeSpec(kind, 300);
    const GeneratedPage& page = cachedPage(spec);
    const QImage mask = test_support::renderInkMask(spec, 300, 0.0);
    BOOST_REQUIRE_MESSAGE(!mask.isNull(), label(spec) << ": null mask");
    BOOST_CHECK_MESSAGE(mask.format() == QImage::Format_Grayscale8, label(spec) << ": mask is not Grayscale8");
    BOOST_CHECK_MESSAGE(mask.size() == page.image.size(), label(spec) << ": mask size differs from the page");

    QRect inkBox;
    long inkPixels = 0;
    bool onlyBlackAndWhite = true;
    for (int y = 0; y < mask.height(); ++y) {
      const uchar* const line = mask.constScanLine(y);
      for (int x = 0; x < mask.width(); ++x) {
        onlyBlackAndWhite = onlyBlackAndWhite && (line[x] == 0 || line[x] == 255);
        if (line[x] == 0) {
          ++inkPixels;
          inkBox = inkBox.isNull() ? QRect(x, y, 1, 1) : inkBox.united(QRect(x, y, 1, 1));
        }
      }
    }
    BOOST_CHECK_MESSAGE(onlyBlackAndWhite, label(spec) << ": the mask holds values other than 0 and 255");
    BOOST_CHECK_MESSAGE(inkPixels > 0, label(spec) << ": the mask has no ink");
    BOOST_CHECK_MESSAGE(page.truth.textBlock.contains(inkBox),
                        label(spec) << ": mask ink box is not inside the text block (specks or picture in the mask?)");
    BOOST_CHECK_MESSAGE(inkBox == page.truth.textBlock, label(spec)
                                                            << ": mask ink box differs from the tight text block");
  }

  // Rendering at another dpi gives the page of that dpi, not a resampled copy.
  const PageSpec at300 = makeSpec(PageKind::FLAT, 300);
  const PageSpec at600 = makeSpec(PageKind::FLAT, 600);
  const QImage upscaled = test_support::renderInkMask(at300, 600, 0.0);
  BOOST_CHECK_MESSAGE(upscaled.size() == QSize(2000, 2300), "mask at 600 dpi has the wrong size");
  BOOST_CHECK_MESSAGE(upscaled == test_support::renderInkMask(at600, 600, 0.0),
                      "mask of the 300 dpi spec at 600 dpi differs from the mask of the 600 dpi spec");

  // An extra rotation about the block centre keeps the amount of ink to within 1 percent.
  const QImage rotated = test_support::renderInkMask(at300, 300, 1.0);
  const QImage plain = test_support::renderInkMask(at300, 300, 0.0);
  long inkRotated = 0;
  long inkPlain = 0;
  for (int y = 0; y < plain.height(); ++y) {
    for (int x = 0; x < plain.width(); ++x) {
      inkPlain += plain.constScanLine(y)[x] == 0;
      inkRotated += rotated.constScanLine(y)[x] == 0;
    }
  }
  BOOST_CHECK_MESSAGE(inkPlain > 0 && std::fabs(double(inkRotated - inkPlain)) < 0.01 * inkPlain,
                      "rotated mask has " << inkRotated << " ink pixels, plain " << inkPlain);
}

BOOST_AUTO_TEST_CASE(test_invalid_dpi_gives_null_page) {
  for (const PageKind kind : kKinds) {
    for (const int dpi : {0, 74, 1201, -300}) {
      PageSpec spec = makeSpec(kind, dpi);
      GeneratedPage page;
      BOOST_CHECK_NO_THROW(page = test_support::generatePage(spec));
      BOOST_CHECK_MESSAGE(page.image.isNull(), label(spec) << ": image is not null");
      BOOST_CHECK_MESSAGE(page.truth.dpi == 0, label(spec) << ": truth.dpi is " << page.truth.dpi);
      BOOST_CHECK_MESSAGE(test_support::renderInkMask(spec, 300, 0.0).isNull(), label(spec)
                                                                                    << ": mask of an invalid spec");
      BOOST_CHECK_MESSAGE(test_support::renderInkMask(makeSpec(kind, 300), dpi, 0.0).isNull(),
                          kindName(kind) << ": mask at the invalid target dpi " << dpi);
    }
    // 75 is the lowest supported dpi.
    const PageSpec lowest = makeSpec(kind, 75);
    const GeneratedPage page = test_support::generatePage(lowest);
    BOOST_CHECK_MESSAGE(!page.image.isNull() && page.truth.dpi == 75, label(lowest) << ": not a valid page");
  }
}

BOOST_AUTO_TEST_CASE(test_forward_map_inverts_renderer_map) {
  for (const PageKind kind : kKinds) {
    for (const int dpi : kDpis) {
      for (const double skew : {0.0, skewFor(kind)}) {
        PageSpec spec = makeSpec(kind, dpi);
        spec.skewDeg = skew;
        // Page size and text block are in the flat frame, so the truth of the skewed page serves both skews.
        const test_support::GroundTruth truth = cachedPage(makeSpec(kind, dpi)).truth;
        const double w = truth.pageSize.width();
        const double h = truth.pageSize.height();
        const std::vector<QPointF> points = {
            QPointF(0.0, 0.0),
            QPointF(w, h),
            QPointF(w / 2.0, h / 2.0),
            QPointF(0.2 * w, 0.3 * h),
            QPointF(0.77 * w, 0.81 * h),
            QPointF(truth.textBlock.left(), truth.textBlock.top()),
            QPointF(truth.textBlock.right(), truth.textBlock.bottom()),
            QPointF(truth.textBlock.center().x() + 0.37, truth.textBlock.center().y() - 0.21),
        };
        for (const QPointF& flat : points) {
          const QPointF image = test_support::mapFlatToImage(spec, flat);
          const QPointF back = test_support::mapImageToFlat(spec, image);
          BOOST_CHECK_MESSAGE(near(back.x(), flat.x(), 1e-6) && near(back.y(), flat.y(), 1e-6),
                              label(spec) << " skew " << skew << ": (" << flat.x() << ", " << flat.y()
                                          << ") came back as (" << back.x() << ", " << back.y() << ")");
        }
      }
    }
  }

  // The renderer and the forward map agree: the centre of an interior ink pixel of the flat page lands on ink.
  for (const PageKind kind : kKinds) {
    const PageSpec spec = makeSpec(kind, 300);
    const GeneratedPage& page = cachedPage(spec);
    const QImage mask = test_support::renderInkMask(spec, 300, 0.0);
    BOOST_REQUIRE_MESSAGE(!mask.isNull() && !page.image.isNull(), label(spec) << ": null image");
    bool found = false;
    for (int y = 1; y < mask.height() - 1 && !found; ++y) {
      for (int x = 1; x < mask.width() - 1 && !found; ++x) {
        bool interior = true;
        for (int dy = -1; dy <= 1 && interior; ++dy) {
          for (int dx = -1; dx <= 1; ++dx) {
            interior = interior && mask.constScanLine(y + dy)[x + dx] == 0;
          }
        }
        if (!interior) {
          continue;
        }
        found = true;
        const QPointF image = test_support::mapFlatToImage(spec, QPointF(x + 0.5, y + 0.5));
        const int px = static_cast<int>(std::floor(image.x()));
        const int py = static_cast<int>(std::floor(image.y()));
        BOOST_REQUIRE_MESSAGE(page.image.rect().contains(px, py), label(spec) << ": mapped point is outside the page");
        BOOST_CHECK_MESSAGE(qGray(page.image.pixel(px, py)) < 128, label(spec) << ": pixel (" << px << ", " << py
                                                                               << ") mapped from flat (" << x << ", "
                                                                               << y << ") is not ink");
      }
    }
    BOOST_CHECK_MESSAGE(found, label(spec) << ": no interior ink pixel in the mask");
  }
}

BOOST_AUTO_TEST_CASE(test_flat_page_unchanged) {
  // Recorded from the plan 02-01 generator before plan 02-03 changed it: the flat page must stay byte-identical.
  PageSpec spec;
  spec.kind = PageKind::FLAT;
  spec.seed = 1;
  spec.skewDeg = 1.5;

  spec.dpi = 300;
  BOOST_CHECK_EQUAL(test_support::fingerprint(test_support::generatePage(spec).image).toStdString(),
                    "befa54089b773ed40c7c3565aac74a9e805e7bb988943c39dec63ad57ffa40a9");
  spec.dpi = 600;
  BOOST_CHECK_EQUAL(test_support::fingerprint(test_support::generatePage(spec).image).toStdString(),
                    "fedb69f8688b445a3ced11c0decdb7da6e62dc5b7a5fd7a5087a37fd5b3615cf");
}

BOOST_AUTO_TEST_CASE(test_pinned_pages_unchanged) {
  // Recorded from the generator before the dpi fit check existed (plan 02-12): the 75 dpi pages of all four kinds
  // and the curved page at 600 dpi must stay byte-identical.  The other kinds at 300 and 600 dpi are pinned by
  // test_flat_page_unchanged and by the fixtures.txt goldens.
  BOOST_CHECK_EQUAL(test_support::fingerprint(cachedPage(makeSpec(PageKind::FLAT, 75)).image).toStdString(),
                    "6a55a26ae3e6c61d7e2843ed8776252777520a4f7a0a0ef9fa7a2403d3128676");
  BOOST_CHECK_EQUAL(test_support::fingerprint(cachedPage(makeSpec(PageKind::SPREAD, 75)).image).toStdString(),
                    "be9fd06ab9bcd1f484e618390a334491c4de029cf450f04e22308a9be9ba084a");
  BOOST_CHECK_EQUAL(test_support::fingerprint(cachedPage(makeSpec(PageKind::MIXED, 75)).image).toStdString(),
                    "69e7489f2618efdaceafc1c753fefda28439a71337813d518bb6021fffaa0458");
  BOOST_CHECK_EQUAL(test_support::fingerprint(cachedPage(makeSpec(PageKind::CURVED, 75)).image).toStdString(),
                    "cddf2d048f0f61cb10d9283730f0c4e9d8138fc6bd41fdea0a7133d9f5485942");
  BOOST_CHECK_EQUAL(test_support::fingerprint(cachedPage(makeSpec(PageKind::CURVED, 600)).image).toStdString(),
                    "d7e6fb18871c11675c4909a2e93275102bac3a8b37768dd82fd86ca771c70096");
}

BOOST_AUTO_TEST_CASE(test_dpi_sweep_gives_valid_or_null_page) {
  // The generator accepts a dpi only where the rounded geometry fits the page and the documented frames.  The
  // expectation below comes from a reproduction of the geometry computation at planning time: FLAT fits at
  // every sweep dpi but 90, the other kinds only at 75 and 200.
  for (const PageKind kind : kKinds) {
    for (const int dpi : kSweepDpis) {
      const PageSpec spec = makeSpec(kind, dpi);
      const GeneratedPage* const page = sweepPage(spec);
      if (!page) {
        continue;
      }
      const QImage maskOfSpec = sweepMask(spec, dpi);
      const QImage maskAtTarget = sweepMask(makeSpec(kind, 300), dpi);

      // Both maps follow the same rule as the page.
      if (sweepExpectsPage(kind, dpi)) {
        BOOST_CHECK_NO_THROW(test_support::mapFlatToImage(spec, QPointF(0.0, 0.0)));
        BOOST_CHECK_NO_THROW(test_support::mapImageToFlat(spec, QPointF(0.0, 0.0)));
      } else {
        BOOST_CHECK_THROW(test_support::mapFlatToImage(spec, QPointF(0.0, 0.0)), std::invalid_argument);
        BOOST_CHECK_THROW(test_support::mapImageToFlat(spec, QPointF(0.0, 0.0)), std::invalid_argument);
      }

      if (!sweepExpectsPage(kind, dpi)) {
        BOOST_CHECK_MESSAGE(page->image.isNull(), label(spec) << ": a page was rendered for a dpi that does not fit");
        BOOST_CHECK_MESSAGE(page->truth.dpi == 0, label(spec) << ": truth.dpi is " << page->truth.dpi);
        BOOST_CHECK_MESSAGE(maskOfSpec.isNull(), label(spec) << ": mask of a dpi that does not fit");
        BOOST_CHECK_MESSAGE(maskAtTarget.isNull(), kindName(kind)
                                                       << ": mask of the 300 dpi spec at target dpi " << dpi);
        continue;
      }

      const test_support::GroundTruth& t = page->truth;
      BOOST_REQUIRE_MESSAGE(!page->image.isNull(), label(spec) << ": null image for a dpi that fits");
      BOOST_CHECK_EQUAL(t.dpi, dpi);
      BOOST_CHECK_MESSAGE(page->image.size() == t.pageSize, label(spec) << ": image size differs from truth.pageSize");
      BOOST_CHECK_MESSAGE(insidePage(t.textBlock, t.pageSize) && !t.textBlock.isEmpty(),
                          label(spec) << ": text block is not inside the page");

      if (kind == PageKind::SPREAD) {
        BOOST_CHECK_MESSAGE(insidePage(t.leftBlock, t.pageSize) && insidePage(t.rightBlock, t.pageSize),
                            label(spec) << ": a block is not inside the page");
        const double gutterX = xAtY(t.gutterCentre, t.pageSize.height() / 2.0);
        BOOST_CHECK_MESSAGE(t.leftBlock.right() < gutterX - t.gutterHalfWidthPx + 1,
                            label(spec) << ": the left block reaches into the gutter band");
        BOOST_CHECK_MESSAGE(t.rightBlock.left() > gutterX + t.gutterHalfWidthPx - 1,
                            label(spec) << ": the right block reaches into the gutter band");
      }

      if (kind == PageKind::MIXED) {
        const int margin = scaled(20, dpi);
        BOOST_CHECK_MESSAGE(insidePage(t.pictureRect, t.pageSize), label(spec) << ": picture is not inside the page");
        BOOST_CHECK_MESSAGE(!t.textBlock.intersects(t.pictureRect), label(spec) << ": picture overlaps the text");
        const QRect farFromText = t.textBlock.adjusted(-margin, -margin, margin, margin);
        const QRect farFromPicture = t.pictureRect.adjusted(-margin, -margin, margin, margin);
        BOOST_CHECK_EQUAL(t.specks.size(), 40u);
        for (size_t i = 0; i < t.specks.size(); ++i) {
          BOOST_CHECK_MESSAGE(insidePage(t.specks[i], t.pageSize), label(spec)
                                                                       << ": speck " << i << " is outside the page");
          BOOST_CHECK_MESSAGE(!t.specks[i].intersects(farFromText) && !t.specks[i].intersects(farFromPicture),
                              label(spec) << ": speck " << i << " is within " << margin << " px of text or picture");
        }
      }

      BOOST_REQUIRE_MESSAGE(!maskOfSpec.isNull(), label(spec) << ": null ink mask for a dpi that fits");
      BOOST_CHECK_MESSAGE(maskOfSpec.format() == QImage::Format_Grayscale8, label(spec) << ": mask is not Grayscale8");
      BOOST_CHECK_MESSAGE(maskOfSpec.size() == t.pageSize, label(spec) << ": mask size differs from the page");
    }
  }
}

BOOST_AUTO_TEST_CASE(test_dpi_support_rule) {
  // The expected numbers were cross-checked against an independent reproduction of the geometry computation (every
  // dpi 75..1200) when the rule was written.  After an intended geometry change, re-pin the accepted counts from the
  // failure message (it prints the count) and re-check the overflow ranges by rendering.  1200 dpi is checked here
  // and not rendered: one page is four times a 600 dpi page and would add minutes to a Debug or ASan run.
  struct Range {
    int first;
    int last;
  };
  const std::map<PageKind, std::vector<Range>> overflow = {
      {PageKind::FLAT, {{90, 90}}},
      {PageKind::SPREAD, {{90, 104}, {150, 157}}},
      {PageKind::MIXED, {{90, 96}}},
      {PageKind::CURVED, {{90, 97}}},
  };
  const std::map<PageKind, int> acceptedCount
      = {{PageKind::FLAT, 1125}, {PageKind::SPREAD, 845}, {PageKind::MIXED, 702}, {PageKind::CURVED, 697}};

  for (const PageKind kind : kKinds) {
    for (const int dpi : {-300, 0, 74, 1201, 1300}) {
      BOOST_CHECK_MESSAGE(!test_support::dpiSupported(kind, dpi), kindName(kind) << ": " << dpi << " dpi is supported");
    }
    BOOST_CHECK_MESSAGE(test_support::dpiSupported(kind, 75), kindName(kind) << ": 75 dpi is not supported");
    for (int dpi = 120; dpi <= 1200; dpi += 60) {
      BOOST_CHECK_MESSAGE(test_support::dpiSupported(kind, dpi), kindName(kind) << ": " << dpi << " dpi is refused");
    }
    for (const Range& range : overflow.at(kind)) {
      for (int dpi = range.first; dpi <= range.last; ++dpi) {
        BOOST_CHECK_MESSAGE(!test_support::dpiSupported(kind, dpi),
                            kindName(kind) << ": " << dpi << " dpi overflowed the page and is supported");
      }
    }
    if (kind != PageKind::FLAT) {
      for (const int dpi : {150, 225, 400, 450}) {
        BOOST_CHECK_MESSAGE(!test_support::dpiSupported(kind, dpi), kindName(kind)
                                                                        << ": " << dpi << " dpi is supported");
      }
    }
    int accepted = 0;
    for (int dpi = 75; dpi <= 1200; ++dpi) {
      accepted += test_support::dpiSupported(kind, dpi) ? 1 : 0;
    }
    BOOST_CHECK_MESSAGE(accepted == acceptedCount.at(kind), kindName(kind)
                                                                << ": " << accepted << " dpi values accepted, expected "
                                                                << acceptedCount.at(kind));
  }
}

BOOST_AUTO_TEST_SUITE_END()
