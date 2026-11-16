// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_FORGEDIMAGES_H_
#define SCANTAILOR_TEST_SUPPORT_FORGEDIMAGES_H_

#include <QByteArray>
#include <QtGlobal>
#include <vector>

namespace test_support {

// One page of a forged TIFF. The header claims width x height; with fullData false the file holds one
// short strip of zero bytes (a few hundred bytes in total, whatever the claimed size), so a reader that trusts
// the header allocates the claimed size and a reader that checks it first does not. With fullData true every
// scanline is written (zeros) and the page decodes. bitsPerSample 1 with one sample is a bilevel page, 8 with
// one sample an 8-bit grey page, 8 with three samples an RGB page.
struct ForgedTiffPage {
  quint32 width;
  quint32 height;
  int bitsPerSample;
  int samplesPerPixel;
  bool fullData;
};

// A little-endian TIFF file, written with libtiff into memory. Synthetic only; nothing touches the disk.
QByteArray forgedTiff(const std::vector<ForgedTiffPage>& pages);

}  // namespace test_support

#endif  // ifndef SCANTAILOR_TEST_SUPPORT_FORGEDIMAGES_H_
