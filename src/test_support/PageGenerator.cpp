// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageGenerator.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "Rng.h"

namespace test_support {

namespace {

// Geometry in base units of 1/300 in; every value is multiplied by dpi / 300.
const int kPageWidth = 1000;
const int kPageHeight = 1150;
const int kDot = 5;        // edge of one glyph dot
const int kGlyphCols = 5;
const int kGlyphRows = 7;
const int kCellDots = 6;   // glyph width plus one dot of letter spacing; a space is one cell
const int kLinePitchDots = 11;
const int kLinesPerPage = 13;
const int kCellsPerLine = 20;
const int kMinWordLen = 2;
const int kMaxWordLen = 9;

const int kPaperRgb[3] = {236, 233, 226};
const int kInkRgb[3] = {32, 30, 34};
const int kNoiseLevels = 3;  // paper noise in [-3, 3]

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

}  // namespace

GeneratedPage generatePage(const PageSpec& spec) {
  if (spec.dpi != 300 && spec.dpi != 600) {
    throw std::invalid_argument("generatePage: dpi must be 300 or 600");
  }
  const double angleRad = spec.skewDeg * kPi / 180.0;
  if (std::fabs(angleRad) > kMaxAngleRad) {
    throw std::invalid_argument("generatePage: |skewDeg| is larger than the supported range");
  }

  const int scale = spec.dpi / 300;
  const int width = kPageWidth * scale;
  const int height = kPageHeight * scale;
  const int dot = kDot * scale;

  // The block is centred as a whole; the maximum ink extent is 119 x 139 dots.
  const int blockWidthDots = kCellsPerLine * kCellDots - 1;
  const int blockHeightDots = (kLinesPerPage - 1) * kLinePitchDots + kGlyphRows;
  const int originX = (kPageWidth - blockWidthDots * kDot) / 2 * scale;
  const int originY = (kPageHeight - blockHeightDots * kDot) / 2 * scale;

  // Flat (unrotated) ink raster.
  std::vector<uint8_t> flat(static_cast<size_t>(width) * static_cast<size_t>(height), 0);
  Rng rng(spec.seed);
  int minX = width;
  int minY = height;
  int maxX = -1;
  int maxY = -1;
  std::vector<int> lineTops;

  for (int line = 0; line < kLinesPerPage; ++line) {
    const int lineTopDots = line * kLinePitchDots;
    lineTops.push_back(originY + lineTopDots * dot);
    int cell = 0;
    while (kCellsPerLine - cell >= kMinWordLen) {
      int wordLen = kMinWordLen + static_cast<int>(rng.below(kMaxWordLen - kMinWordLen + 1));
      wordLen = std::min(wordLen, kCellsPerLine - cell);
      for (int i = 0; i < wordLen; ++i, ++cell) {
        const int letter = static_cast<int>(rng.below(26));
        for (int row = 0; row < kGlyphRows; ++row) {
          for (int col = 0; col < kGlyphCols; ++col) {
            if (kGlyphs[letter][row][col] != '#') {
              continue;
            }
            const int x0 = originX + (cell * kCellDots + col) * dot;
            const int y0 = originY + (lineTopDots + row) * dot;
            for (int dy = 0; dy < dot; ++dy) {
              for (int dx = 0; dx < dot; ++dx) {
                flat[static_cast<size_t>(y0 + dy) * static_cast<size_t>(width) + static_cast<size_t>(x0 + dx)] = 1;
              }
            }
            minX = std::min(minX, x0);
            minY = std::min(minY, y0);
            maxX = std::max(maxX, x0 + dot - 1);
            maxY = std::max(maxY, y0 + dot - 1);
          }
        }
      }
      cell += 1;  // the space after the word
    }
  }

  // Rotate by inverse mapping with 3x3 supersampling.
  const double cs = taylorCos(angleRad);
  const double sn = taylorSin(angleRad);
  const double centreX = width / 2.0;
  const double centreY = height / 2.0;

  QImage image(width, height, QImage::Format_RGB32);
  for (int y = 0; y < height; ++y) {
    QRgb* const out = reinterpret_cast<QRgb*>(image.scanLine(y));
    for (int x = 0; x < width; ++x) {
      int inkCount = 0;
      for (int sy = 0; sy < 3; ++sy) {
        for (int sx = 0; sx < 3; ++sx) {
          const double dx = x + (2 * sx + 1) / 6.0 - centreX;
          const double dy = y + (2 * sy + 1) / 6.0 - centreY;
          const double fx = cs * dx + sn * dy + centreX;
          const double fy = -sn * dx + cs * dy + centreY;
          const int ix = static_cast<int>(std::floor(fx));
          const int iy = static_cast<int>(std::floor(fy));
          if (ix >= 0 && ix < width && iy >= 0 && iy < height
              && flat[static_cast<size_t>(iy) * static_cast<size_t>(width) + static_cast<size_t>(ix)]) {
            ++inkCount;
          }
        }
      }
      const int noise = paperNoise(x, y, spec.seed);
      int rgb[3];
      for (int c = 0; c < 3; ++c) {
        const int blended = (kPaperRgb[c] * (9 - inkCount) + kInkRgb[c] * inkCount + 4) / 9;
        rgb[c] = clampByte(blended + noise);
      }
      out[x] = qRgb(rgb[0], rgb[1], rgb[2]);
    }
  }
  const int dotsPerMeter = static_cast<int>(spec.dpi / 0.0254 + 0.5);
  image.setDotsPerMeterX(dotsPerMeter);
  image.setDotsPerMeterY(dotsPerMeter);

  GroundTruth truth;
  truth.dpi = spec.dpi;
  truth.skewDeg = spec.skewDeg;
  truth.pageSize = QSize(width, height);
  truth.textBlock = QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
  truth.lineCount = kLinesPerPage;
  truth.lineTops = lineTops;

  GeneratedPage page;
  page.image = image;
  page.truth = truth;
  return page;
}

}  // namespace test_support
