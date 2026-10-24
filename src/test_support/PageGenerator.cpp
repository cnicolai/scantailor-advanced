// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageGenerator.h"

#include <QCryptographicHash>
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "Rng.h"

namespace test_support {

namespace {

// Geometry in base units of 1/300 in; every value is scaled by dpi / 300 (see scaled()).  The fixture
// designs are described in PageGenerator.h.
const int kStandardPageWidth = 1000;
const int kStandardPageHeight = 1150;
const int kSpreadPageWidth = 1654;
const int kSpreadPageHeight = 1181;

const int kDot = 5;  // edge of one glyph dot
const int kGlyphCols = 5;
const int kGlyphRows = 7;
const int kCellDots = 6;  // glyph width plus one dot of letter spacing; a space is one cell
const int kLinePitchDots = 11;
const int kMinWordLen = 2;
const int kMaxWordLen = 9;

const int kPaperRgb[3] = {236, 233, 226};
const int kInkRgb[3] = {32, 30, 34};
const int kNoiseLevels = 3;  // paper noise in [-3, 3]

const int kMinDpi = 75;
const int kMaxDpi = 1200;

// SPREAD
const int kSpreadBlockCells = 21;
const int kSpreadBlockLines = 15;
const int kSpreadBlockFrameWidth = 640;
const int kSpreadBlockFrameHeight = 860;
const int kSpreadLeftX = 73;
const int kSpreadRightX = 940;
const int kSpreadTopY = 160;
const double kSpreadGutterX = 826.5;
const double kSpreadGutterHalfWidth = 113.5;
const double kSpreadShadowHalfWidth = 60.0;
const double kSpreadShadowDepth = 25.0;  // paper levels at the gutter centre

// MIXED
const int kMixedBlockCells = 14;
const int kMixedBlockLines = 15;
const int kMixedBlockX = 100;
const int kMixedBlockY = 150;
const int kMixedBlockFrameWidth = 420;
const int kMixedBlockFrameHeight = 850;
const int kPictureX = 580;
const int kPictureY = 300;
const int kPictureWidth = 320;
const int kPictureHeight = 400;
const int kPictureCell = 16;
const int kPictureMinLevel = 40;
const int kPictureLevels = 181;  // 40..220
const int kSpeckCount = 40;
const int kSpeckMaxSide = 3;
const int kSpeckClearance = 20;  // from text, picture and page edge
const int kSpeckGap = 4;         // between specks
const int kSpeckMaxAttempts = 100000;

// CURVED
const int kCurvedBlockCells = 23;
const int kCurvedBlockLines = 14;
const int kCurvedBlockFrameWidth = 700;
const int kCurvedBlockFrameHeight = 800;
const double kCurlAmplitude = 25.0;
const double kCurlCompression = 0.15;

// FLAT
const int kFlatBlockCells = 20;
const int kFlatBlockLines = 13;

const double kPi = 3.14159265358979323846;
const double kMaxAngleRad = 0.1;

// 5x7 bitmap font, lowercase only.  '#' is ink.
const char* const kGlyphs[26][kGlyphRows] = {
    {".....", ".....", ".###.", "....#", ".####", "#...#", ".####"},  // a
    {"#....", "#....", "####.", "#...#", "#...#", "#...#", "####."},  // b
    {".....", ".....", ".###.", "#...#", "#....", "#...#", ".###."},  // c
    {"....#", "....#", ".####", "#...#", "#...#", "#...#", ".####"},  // d
    {".....", ".....", ".###.", "#...#", "#####", "#....", ".###."},  // e
    {"..##.", ".#..#", ".#...", "###..", ".#...", ".#...", ".#..."},  // f
    {".....", ".####", "#...#", "#...#", ".####", "....#", ".###."},  // g
    {"#....", "#....", "####.", "#...#", "#...#", "#...#", "#...#"},  // h
    {"..#..", ".....", ".##..", "..#..", "..#..", "..#..", ".###."},  // i
    {"...#.", ".....", "..##.", "...#.", "...#.", "#..#.", ".##.."},  // j
    {"#....", "#....", "#..#.", "#.#..", "##...", "#.#..", "#..#."},  // k
    {".##..", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."},  // l
    {".....", ".....", "##.#.", "#.#.#", "#.#.#", "#...#", "#...#"},  // m
    {".....", ".....", "####.", "#...#", "#...#", "#...#", "#...#"},  // n
    {".....", ".....", ".###.", "#...#", "#...#", "#...#", ".###."},  // o
    {".....", "####.", "#...#", "#...#", "####.", "#....", "#...."},  // p
    {".....", ".####", "#...#", "#...#", ".####", "....#", "....#"},  // q
    {".....", ".....", "#.##.", "##..#", "#....", "#....", "#...."},  // r
    {".....", ".....", ".####", "#....", ".###.", "....#", "####."},  // s
    {".#...", ".#...", "###..", ".#...", ".#...", ".#..#", "..##."},  // t
    {".....", ".....", "#...#", "#...#", "#...#", "#..##", ".##.#"},  // u
    {".....", ".....", "#...#", "#...#", "#...#", ".#.#.", "..#.."},  // v
    {".....", ".....", "#...#", "#...#", "#.#.#", "#.#.#", ".#.#."},  // w
    {".....", ".....", "#...#", ".#.#.", "..#..", ".#.#.", "#...#"},  // x
    {".....", "#...#", "#...#", "#...#", ".####", "....#", ".###."},  // y
    {".....", ".....", "#####", "...#.", "..#..", ".#...", "#####"},  // z
};

// Series for small angles only; they diverge quickly for larger arguments.
double taylorSin(double a) {
  double term = a;
  double sum = a;
  for (int k = 1; k < 12; ++k) {
    term *= -a * a / ((2.0 * k) * (2.0 * k + 1.0));
    sum += term;
  }
  return sum;
}

double taylorCos(double a) {
  double term = 1.0;
  double sum = 1.0;
  for (int k = 1; k < 12; ++k) {
    term *= -a * a / ((2.0 * k - 1.0) * (2.0 * k));
    sum += term;
  }
  return sum;
}

// Pure hash of (x, y, seed) to a small signed level.
int paperNoise(int x, int y, uint64_t seed) {
  const uint64_t mixed = seed ^ (static_cast<uint64_t>(static_cast<uint32_t>(x)) * 0x9E3779B97F4A7C15ULL)
                         ^ (static_cast<uint64_t>(static_cast<uint32_t>(y)) * 0xC2B2AE3D27D4EB4FULL);
  Rng rng(mixed);
  return static_cast<int>(rng.next() % (2 * kNoiseLevels + 1)) - kNoiseLevels;
}

int clampByte(int v) {
  return std::max(0, std::min(255, v));
}

bool dpiInRange(const int dpi) {
  return dpi >= kMinDpi && dpi <= kMaxDpi;
}

/// Base units to pixels.  Exact at 300 and 600 dpi, rounded to the nearest pixel elsewhere.
int scaled(const int base, const int dpi) {
  return (base * dpi + 150) / 300;
}

double scaledExact(const double base, const int dpi) {
  return base * dpi / 300.0;
}

void checkAngle(const double angleDeg) {
  if (std::fabs(angleDeg * kPi / 180.0) > kMaxAngleRad) {
    throw std::invalid_argument("test_support: |angle| is larger than the supported range");
  }
}

struct TextBlockGeometry {
  int originX;  // pixels, top-left corner of the first glyph
  int originY;
  int cells;
  int lines;
  bool spaceFree;  // every glyph cell holds a letter

  int inkWidth(const int dot) const { return (cells * kCellDots - 1) * dot; }
  int inkHeight(const int dot) const { return ((lines - 1) * kLinePitchDots + kGlyphRows) * dot; }
};

/// Everything that does not depend on the seed.
struct Geometry {
  PageKind kind = PageKind::FLAT;
  int dpi = 300;
  int width = 0;
  int height = 0;
  int dot = 0;
  std::vector<TextBlockGeometry> blocks;
  QRect pictureRect;     // MIXED
  double gutterX = 0.0;  // SPREAD, flat frame
  double gutterHalfWidth = 0.0;
  double shadowHalfWidth = 0.0;  // 0 = no shadow
  double shadowDepth = 0.0;
  double curlAmplitude = 0.0;  // CURVED, pixels; 0 = no curl
  double curlFar = 0.0;        // x of the far edge of the block
  double curlSpine = 0.0;      // x of the spine edge
  double curlTop = 0.0;        // y of the top line
  double curlHeight = 0.0;     // y extent of the block
};

TextBlockGeometry centredBlock(const int cells,
                               const int lines,
                               const bool spaceFree,
                               const int pageWidth,
                               const int pageHeight,
                               const int dpi) {
  const int inkWidthBase = (cells * kCellDots - 1) * kDot;
  const int inkHeightBase = ((lines - 1) * kLinePitchDots + kGlyphRows) * kDot;
  TextBlockGeometry block;
  block.originX = scaled((pageWidth - inkWidthBase) / 2, dpi);
  block.originY = scaled((pageHeight - inkHeightBase) / 2, dpi);
  block.cells = cells;
  block.lines = lines;
  block.spaceFree = spaceFree;
  return block;
}

Geometry computeGeometry(const PageKind kind, const int dpi) {
  Geometry g;
  g.kind = kind;
  g.dpi = dpi;
  g.dot = scaled(kDot, dpi);
  const bool spread = kind == PageKind::SPREAD;
  g.width = scaled(spread ? kSpreadPageWidth : kStandardPageWidth, dpi);
  g.height = scaled(spread ? kSpreadPageHeight : kStandardPageHeight, dpi);

  switch (kind) {
    case PageKind::FLAT:
      g.blocks.push_back(
          centredBlock(kFlatBlockCells, kFlatBlockLines, false, kStandardPageWidth, kStandardPageHeight, dpi));
      break;
    case PageKind::SPREAD: {
      TextBlockGeometry left
          = {scaled(kSpreadLeftX, dpi), scaled(kSpreadTopY, dpi), kSpreadBlockCells, kSpreadBlockLines, false};
      TextBlockGeometry right = left;
      right.originX = scaled(kSpreadRightX, dpi);
      g.blocks.push_back(left);
      g.blocks.push_back(right);
      g.gutterX = scaledExact(kSpreadGutterX, dpi);
      g.gutterHalfWidth = scaledExact(kSpreadGutterHalfWidth, dpi);
      g.shadowHalfWidth = scaledExact(kSpreadShadowHalfWidth, dpi);
      g.shadowDepth = kSpreadShadowDepth;
      break;
    }
    case PageKind::MIXED: {
      TextBlockGeometry block
          = {scaled(kMixedBlockX, dpi), scaled(kMixedBlockY, dpi), kMixedBlockCells, kMixedBlockLines, false};
      g.blocks.push_back(block);
      g.pictureRect = QRect(scaled(kPictureX, dpi), scaled(kPictureY, dpi), scaled(kPictureWidth, dpi),
                            scaled(kPictureHeight, dpi));
      break;
    }
    case PageKind::CURVED: {
      const TextBlockGeometry block
          = centredBlock(kCurvedBlockCells, kCurvedBlockLines, true, kStandardPageWidth, kStandardPageHeight, dpi);
      g.blocks.push_back(block);
      g.curlAmplitude = scaledExact(kCurlAmplitude, dpi);
      g.curlFar = block.originX;
      g.curlSpine = block.originX + block.inkWidth(g.dot);
      g.curlTop = block.originY;
      g.curlHeight = block.inkHeight(g.dot);
      break;
    }
  }
  return g;
}

/// Whether the rounded geometry keeps every text block inside the page and inside its documented frame, and the
/// MIXED picture inside the page.  The first condition is what keeps the glyph writes inside the raster; the
/// frames keep the ground truth honest (the SPREAD gutter band starts where the left frame ends, MIXED speck
/// clearance is measured from the MIXED frame).  FLAT documents no frame.
bool layoutFits(const Geometry& g) {
  const QRect page(0, 0, g.width, g.height);
  for (const TextBlockGeometry& block : g.blocks) {
    const QRect ink(block.originX, block.originY, block.inkWidth(g.dot), block.inkHeight(g.dot));
    if (!page.contains(ink)) {
      return false;
    }
    QRect frame;
    switch (g.kind) {
      case PageKind::FLAT:
        break;
      case PageKind::SPREAD:
        frame = QRect(block.originX, block.originY, scaled(kSpreadBlockFrameWidth, g.dpi),
                      scaled(kSpreadBlockFrameHeight, g.dpi));
        break;
      case PageKind::MIXED:
        frame = QRect(block.originX, block.originY, scaled(kMixedBlockFrameWidth, g.dpi),
                      scaled(kMixedBlockFrameHeight, g.dpi));
        break;
      case PageKind::CURVED:
        frame = QRect(scaled((kStandardPageWidth - kCurvedBlockFrameWidth) / 2, g.dpi),
                      scaled((kStandardPageHeight - kCurvedBlockFrameHeight) / 2, g.dpi),
                      scaled(kCurvedBlockFrameWidth, g.dpi), scaled(kCurvedBlockFrameHeight, g.dpi));
        break;
    }
    if (!frame.isNull() && !frame.contains(ink)) {
      return false;
    }
  }
  return g.kind != PageKind::MIXED || page.contains(g.pictureRect);
}

/**
 * The geometric maps of a page kind.  imageToFlat is what the renderer samples with: first the inverse
 * rotation about the page centre, then for a CURVED page the inverse curl.  flatToImage is its inverse.
 */
class Mapper {
 public:
  Mapper(const Geometry& g, const double skewDeg, const double centreX, const double centreY)
      : m_g(g), m_centreX(centreX), m_centreY(centreY) {
    checkAngle(skewDeg);
    const double angleRad = skewDeg * kPi / 180.0;
    m_cos = taylorCos(angleRad);
    m_sin = taylorSin(angleRad);
  }

  void imageToFlat(const double imageX, const double imageY, double& flatX, double& flatY) const {
    const double dx = imageX - m_centreX;
    const double dy = imageY - m_centreY;
    const double rx = m_cos * dx + m_sin * dy + m_centreX;
    const double ry = -m_sin * dx + m_cos * dy + m_centreY;
    if (m_g.curlAmplitude == 0.0) {
      flatX = rx;
      flatY = ry;
      return;
    }
    // The profile is evaluated at the image x coordinate, which makes both x and y closed forms.
    const double a = m_g.curlAmplitude;
    const double p = curlProfile(rx);
    flatX = rx - kCurlCompression * a * p;
    flatY = (ry + a * p * (2.0 * m_g.curlTop / m_g.curlHeight + 1.0)) / (1.0 + 2.0 * a * p / m_g.curlHeight);
  }

  void flatToImage(const double flatX, const double flatY, double& imageX, double& imageY) const {
    double rx = flatX;
    double ry = flatY;
    if (m_g.curlAmplitude != 0.0) {
      const double a = m_g.curlAmplitude;
      // Fixed-point iteration for x = X - 0.15 A p(X); the step is a contraction except within 0.01 px of
      // the spine edge.
      double x = flatX;
      for (int i = 0; i < 20; ++i) {
        const double next = flatX + kCurlCompression * a * curlProfile(x);
        const bool converged = std::fabs(next - x) < 1e-12;
        x = next;
        if (converged) {
          break;
        }
      }
      const double p = curlProfile(x);
      const double v = (flatY - m_g.curlTop) / m_g.curlHeight;
      rx = x;
      ry = flatY + a * p * (2.0 * v - 1.0);
    }
    const double dx = rx - m_centreX;
    const double dy = ry - m_centreY;
    imageX = m_cos * dx - m_sin * dy + m_centreX;
    imageY = m_sin * dx + m_cos * dy + m_centreY;
  }

 private:
  // 0 at the far edge, 1 at the spine edge: a circular arc, not a parabola.
  double curlProfile(const double x) const {
    double u = (x - m_g.curlFar) / (m_g.curlSpine - m_g.curlFar);
    u = std::max(0.0, std::min(1.0, u));
    return 1.0 - std::sqrt(1.0 - u * u);
  }

  const Geometry& m_g;
  double m_centreX;
  double m_centreY;
  double m_cos;
  double m_sin;
};

/// Marks in the flat raster.
enum RasterMark : uint8_t { PAPER = 0, TEXT_INK = 1, SPECK_INK = 2 };

struct Extent {
  int minX;
  int minY;
  int maxX;
  int maxY;

  Extent() : minX(1 << 30), minY(1 << 30), maxX(-1), maxY(-1) {}

  void add(const int x0, const int y0, const int x1, const int y1) {
    minX = std::min(minX, x0);
    minY = std::min(minY, y0);
    maxX = std::max(maxX, x1);
    maxY = std::max(maxY, y1);
  }

  QRect rect() const { return maxX < 0 ? QRect() : QRect(minX, minY, maxX - minX + 1, maxY - minY + 1); }
};

/// The seed dependent part of a page: the flat ink raster and the ground truth read from it.
struct Layout {
  Geometry g;
  std::vector<uint8_t> raster;  // width * height RasterMarks, flat frame
  std::vector<QRect> blockInk;  // tight ink box per text block
  std::vector<int> lineTops;    // first block
  std::vector<QRect> specks;

  size_t index(const int x, const int y) const {
    return static_cast<size_t>(y) * static_cast<size_t>(g.width) + static_cast<size_t>(x);
  }

  RasterMark markAt(const int x, const int y) const {
    if (x < 0 || x >= g.width || y < 0 || y >= g.height) {
      return PAPER;
    }
    return static_cast<RasterMark>(raster[index(x, y)]);
  }
};

void placeGlyph(Layout& layout,
                const TextBlockGeometry& block,
                const int line,
                const int cell,
                const int letter,
                Extent& extent) {
  const int dot = layout.g.dot;
  const int lineTopDots = line * kLinePitchDots;
  for (int row = 0; row < kGlyphRows; ++row) {
    for (int col = 0; col < kGlyphCols; ++col) {
      if (kGlyphs[letter][row][col] != '#') {
        continue;
      }
      const int x0 = block.originX + (cell * kCellDots + col) * dot;
      const int y0 = block.originY + (lineTopDots + row) * dot;
      // Backstop behind dpiSupported(): unreachable for a supported dpi.
      if (x0 < 0 || y0 < 0 || x0 + dot > layout.g.width || y0 + dot > layout.g.height) {
        throw std::logic_error("test_support: glyph dot outside the raster");
      }
      for (int dy = 0; dy < dot; ++dy) {
        for (int dx = 0; dx < dot; ++dx) {
          layout.raster[layout.index(x0 + dx, y0 + dy)] = TEXT_INK;
        }
      }
      extent.add(x0, y0, x0 + dot - 1, y0 + dot - 1);
    }
  }
}

QRect placeTextBlock(Layout& layout, const TextBlockGeometry& block, Rng& rng, std::vector<int>* lineTops) {
  Extent extent;
  const int dot = layout.g.dot;
  for (int line = 0; line < block.lines; ++line) {
    if (lineTops) {
      lineTops->push_back(block.originY + line * kLinePitchDots * dot);
    }
    if (block.spaceFree) {
      for (int cell = 0; cell < block.cells; ++cell) {
        placeGlyph(layout, block, line, cell, static_cast<int>(rng.below(26)), extent);
      }
      continue;
    }
    int cell = 0;
    while (block.cells - cell >= kMinWordLen) {
      int wordLen = kMinWordLen + static_cast<int>(rng.below(kMaxWordLen - kMinWordLen + 1));
      wordLen = std::min(wordLen, block.cells - cell);
      for (int i = 0; i < wordLen; ++i, ++cell) {
        placeGlyph(layout, block, line, cell, static_cast<int>(rng.below(26)), extent);
      }
      cell += 1;  // the space after the word
    }
  }
  return extent.rect();
}

void placeSpecks(Layout& layout, const uint64_t seed) {
  const Geometry& g = layout.g;
  const int dpi = g.dpi;
  const int clearance = scaled(kSpeckClearance, dpi);
  const int gap = scaled(kSpeckGap, dpi);
  const TextBlockGeometry& block = g.blocks.front();
  const QRect textFrame(block.originX, block.originY, scaled(kMixedBlockFrameWidth, dpi),
                        scaled(kMixedBlockFrameHeight, dpi));
  const QRect avoidText = textFrame.adjusted(-clearance, -clearance, clearance, clearance);
  const QRect avoidPicture = g.pictureRect.adjusted(-clearance, -clearance, clearance, clearance);

  Rng rng(seed ^ 0x5EC75EC75EC75EC7ULL);
  for (int i = 0; i < kSpeckCount; ++i) {
    bool placed = false;
    for (int attempt = 0; attempt < kSpeckMaxAttempts && !placed; ++attempt) {
      const int side = scaled(1 + static_cast<int>(rng.below(kSpeckMaxSide)), dpi);
      const int xRange = g.width - 2 * clearance - side + 1;
      const int yRange = g.height - 2 * clearance - side + 1;
      const QRect candidate(clearance + static_cast<int>(rng.below(static_cast<uint32_t>(xRange))),
                            clearance + static_cast<int>(rng.below(static_cast<uint32_t>(yRange))), side, side);
      if (candidate.intersects(avoidText) || candidate.intersects(avoidPicture)) {
        continue;
      }
      const QRect spaced = candidate.adjusted(-gap, -gap, gap, gap);
      bool free = true;
      for (const QRect& other : layout.specks) {
        free = free && !spaced.intersects(other);
      }
      if (!free) {
        continue;
      }
      layout.specks.push_back(candidate);
      placed = true;
    }
    if (!placed) {
      throw std::runtime_error("test_support: no room for a speck");
    }
  }
  for (const QRect& speck : layout.specks) {
    // Backstop as in placeGlyph.  Not QRect::contains: a speck of side 0 (1 base px at 75 dpi) is a null rectangle
    // that writes nothing.
    if (speck.left() < 0 || speck.top() < 0 || speck.right() >= g.width || speck.bottom() >= g.height) {
      throw std::logic_error("test_support: speck outside the raster");
    }
    for (int y = speck.top(); y <= speck.bottom(); ++y) {
      for (int x = speck.left(); x <= speck.right(); ++x) {
        layout.raster[layout.index(x, y)] = SPECK_INK;
      }
    }
  }
}

Layout buildLayout(const PageKind kind, const int dpi, const uint64_t seed) {
  Layout layout;
  layout.g = computeGeometry(kind, dpi);
  layout.raster.assign(static_cast<size_t>(layout.g.width) * static_cast<size_t>(layout.g.height), PAPER);

  Rng rng(seed);
  for (size_t i = 0; i < layout.g.blocks.size(); ++i) {
    layout.blockInk.push_back(placeTextBlock(layout, layout.g.blocks[i], rng, i == 0 ? &layout.lineTops : nullptr));
  }
  if (kind == PageKind::MIXED) {
    placeSpecks(layout, seed);
  }
  return layout;
}

QRect textBlockOf(const Layout& layout) {
  QRect all;
  for (const QRect& ink : layout.blockInk) {
    all = all.isNull() ? ink : all.united(ink);
  }
  return all;
}

/// Colour value noise for the picture: a grid of hashed colours, bilinear in between.
class PictureNoise {
 public:
  PictureNoise(const QRect& rect, const int cell, const uint64_t seed) : m_rect(rect), m_cell(cell) {
    m_cols = rect.width() / cell + 2;
    m_rows = rect.height() / cell + 2;
    m_nodes.resize(static_cast<size_t>(m_cols) * static_cast<size_t>(m_rows) * 3);
    for (int iy = 0; iy < m_rows; ++iy) {
      for (int ix = 0; ix < m_cols; ++ix) {
        for (int ch = 0; ch < 3; ++ch) {
          Rng rng(seed ^ (static_cast<uint64_t>(ix) * 0xA24BAED4963EE407ULL)
                  ^ (static_cast<uint64_t>(iy) * 0x9FB21C651E98DF25ULL)
                  ^ (static_cast<uint64_t>(ch + 1) * 0xD6E8FEB86659FD93ULL));
          m_nodes[node(ix, iy, ch)] = kPictureMinLevel + static_cast<int>(rng.below(kPictureLevels));
        }
      }
    }
  }

  bool contains(const double x, const double y) const {
    return x >= m_rect.left() && x < m_rect.left() + m_rect.width() && y >= m_rect.top()
           && y < m_rect.top() + m_rect.height();
  }

  void sample(const double x, const double y, int rgb[3]) const {
    const double ux = (x - m_rect.left()) / m_cell;
    const double uy = (y - m_rect.top()) / m_cell;
    const int ix = static_cast<int>(std::floor(ux));
    const int iy = static_cast<int>(std::floor(uy));
    const double tx = ux - ix;
    const double ty = uy - iy;
    for (int ch = 0; ch < 3; ++ch) {
      const double v00 = m_nodes[node(ix, iy, ch)];
      const double v10 = m_nodes[node(ix + 1, iy, ch)];
      const double v01 = m_nodes[node(ix, iy + 1, ch)];
      const double v11 = m_nodes[node(ix + 1, iy + 1, ch)];
      const double top = v00 + (v10 - v00) * tx;
      const double bottom = v01 + (v11 - v01) * tx;
      rgb[ch] = static_cast<int>(top + (bottom - top) * ty + 0.5);
    }
  }

 private:
  size_t node(const int ix, const int iy, const int ch) const {
    return (static_cast<size_t>(iy) * static_cast<size_t>(m_cols) + static_cast<size_t>(ix)) * 3
           + static_cast<size_t>(ch);
  }

  QRect m_rect;
  int m_cell;
  int m_cols;
  int m_rows;
  std::vector<int> m_nodes;
};

}  // namespace

bool dpiSupported(const PageKind kind, const int dpi) {
  return dpiInRange(dpi) && layoutFits(computeGeometry(kind, dpi));
}

GeneratedPage generatePage(const PageSpec& spec) {
  GeneratedPage page;
  if (!dpiSupported(spec.kind, spec.dpi)) {
    return page;
  }
  checkAngle(spec.skewDeg);

  const Layout layout = buildLayout(spec.kind, spec.dpi, spec.seed);
  const Geometry& g = layout.g;
  const int width = g.width;
  const int height = g.height;
  const Mapper mapper(g, spec.skewDeg, width / 2.0, height / 2.0);

  std::unique_ptr<PictureNoise> picture;
  if (!g.pictureRect.isNull()) {
    picture.reset(new PictureNoise(g.pictureRect, scaled(kPictureCell, spec.dpi), spec.seed));
  }

  // Inverse mapping with 3x3 supersampling.
  QImage image(width, height, QImage::Format_RGB32);
  for (int y = 0; y < height; ++y) {
    QRgb* const out = reinterpret_cast<QRgb*>(image.scanLine(y));
    for (int x = 0; x < width; ++x) {
      int sum[3] = {0, 0, 0};
      for (int sy = 0; sy < 3; ++sy) {
        for (int sx = 0; sx < 3; ++sx) {
          double fx;
          double fy;
          mapper.imageToFlat(x + (2 * sx + 1) / 6.0, y + (2 * sy + 1) / 6.0, fx, fy);
          const int ix = static_cast<int>(std::floor(fx));
          const int iy = static_cast<int>(std::floor(fy));
          if (layout.markAt(ix, iy) != PAPER) {
            for (int c = 0; c < 3; ++c) {
              sum[c] += kInkRgb[c];
            }
          } else if (picture && picture->contains(fx, fy)) {
            int rgb[3];
            picture->sample(fx, fy, rgb);
            for (int c = 0; c < 3; ++c) {
              sum[c] += rgb[c];
            }
          } else {
            int dark = 0;
            if (g.shadowHalfWidth > 0.0) {
              const double d = std::fabs(fx - g.gutterX) / g.shadowHalfWidth;
              if (d < 1.0) {
                dark = static_cast<int>(g.shadowDepth * (1.0 - d * d) + 0.5);
              }
            }
            for (int c = 0; c < 3; ++c) {
              sum[c] += kPaperRgb[c] - dark;
            }
          }
        }
      }
      const int noise = paperNoise(x, y, spec.seed);
      out[x] = qRgb(clampByte((sum[0] + 4) / 9 + noise), clampByte((sum[1] + 4) / 9 + noise),
                    clampByte((sum[2] + 4) / 9 + noise));
    }
  }
  const int dotsPerMeter = static_cast<int>(spec.dpi / 0.0254 + 0.5);
  image.setDotsPerMeterX(dotsPerMeter);
  image.setDotsPerMeterY(dotsPerMeter);

  GroundTruth truth;
  truth.dpi = spec.dpi;
  truth.skewDeg = spec.skewDeg;
  truth.pageSize = QSize(width, height);
  truth.textBlock = textBlockOf(layout);
  truth.lineCount = g.blocks.front().lines;
  truth.lineTops = layout.lineTops;
  if (spec.kind == PageKind::SPREAD) {
    truth.leftBlock = layout.blockInk[0];
    truth.rightBlock = layout.blockInk[1];
    double topX;
    double topY;
    double bottomX;
    double bottomY;
    mapper.flatToImage(g.gutterX, 0.0, topX, topY);
    mapper.flatToImage(g.gutterX, height, bottomX, bottomY);
    truth.gutterCentre = QLineF(topX, topY, bottomX, bottomY);
    truth.gutterHalfWidthPx = g.gutterHalfWidth;
  }
  truth.pictureRect = g.pictureRect;
  truth.specks = layout.specks;
  truth.curlAmplitudePx = g.curlAmplitude;

  page.image = image;
  page.truth = truth;
  return page;
}

QImage renderInkMask(const PageSpec& spec, const int targetDpi, const double extraRotationDeg) {
  checkAngle(extraRotationDeg);
  if (!dpiSupported(spec.kind, spec.dpi) || !dpiSupported(spec.kind, targetDpi)) {
    return QImage();
  }
  // The text depends on the seed only, so the layout at the target dpi is the page at that dpi.
  const Layout layout = buildLayout(spec.kind, targetDpi, spec.seed);
  const Geometry& g = layout.g;
  const QRect block = textBlockOf(layout);
  const double centreX = block.left() + block.width() / 2.0;
  const double centreY = block.top() + block.height() / 2.0;

  const double angleRad = extraRotationDeg * kPi / 180.0;
  const double cs = taylorCos(angleRad);
  const double sn = taylorSin(angleRad);

  QImage mask(g.width, g.height, QImage::Format_Grayscale8);
  for (int y = 0; y < g.height; ++y) {
    uchar* const out = mask.scanLine(y);
    for (int x = 0; x < g.width; ++x) {
      int inkCount = 0;
      for (int sy = 0; sy < 3; ++sy) {
        for (int sx = 0; sx < 3; ++sx) {
          const double dx = x + (2 * sx + 1) / 6.0 - centreX;
          const double dy = y + (2 * sy + 1) / 6.0 - centreY;
          const double fx = cs * dx + sn * dy + centreX;
          const double fy = -sn * dx + cs * dy + centreY;
          if (layout.markAt(static_cast<int>(std::floor(fx)), static_cast<int>(std::floor(fy))) == TEXT_INK) {
            ++inkCount;
          }
        }
      }
      out[x] = (2 * inkCount >= 9) ? 0 : 255;
    }
  }
  return mask;
}

QByteArray fingerprint(const QImage& image) {
  if (image.isNull()) {
    return QByteArray();
  }
  const QImage rgb = (image.format() == QImage::Format_RGB32) ? image : image.convertToFormat(QImage::Format_RGB32);
  QCryptographicHash hash(QCryptographicHash::Sha256);
  for (int y = 0; y < rgb.height(); ++y) {
    const char* const line = reinterpret_cast<const char*>(rgb.constScanLine(y));
    hash.addData(QByteArrayView(line, static_cast<qsizetype>(rgb.width()) * 4));
  }
  return hash.result().toHex();
}

QPointF mapFlatToImage(const PageSpec& spec, const QPointF& flatPoint) {
  if (!dpiSupported(spec.kind, spec.dpi)) {
    throw std::invalid_argument("mapFlatToImage: unsupported dpi");
  }
  const Geometry g = computeGeometry(spec.kind, spec.dpi);
  const Mapper mapper(g, spec.skewDeg, g.width / 2.0, g.height / 2.0);
  double x;
  double y;
  mapper.flatToImage(flatPoint.x(), flatPoint.y(), x, y);
  return QPointF(x, y);
}

QPointF mapImageToFlat(const PageSpec& spec, const QPointF& imagePoint) {
  if (!dpiSupported(spec.kind, spec.dpi)) {
    throw std::invalid_argument("mapImageToFlat: unsupported dpi");
  }
  const Geometry g = computeGeometry(spec.kind, spec.dpi);
  const Mapper mapper(g, spec.skewDeg, g.width / 2.0, g.height / 2.0);
  double x;
  double y;
  mapper.imageToFlat(imagePoint.x(), imagePoint.y(), x, y);
  return QPointF(x, y);
}

}  // namespace test_support
