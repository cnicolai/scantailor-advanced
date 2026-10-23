// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageCompare.h"

namespace test_support {

// Interface stub for the RED commit of plan 02-02 task 1: the tests compile and fail on their assertions.
CompareResult compareImages(const QImage&, const QImage&, ImageKind) {
  return CompareResult{false, {}, QImage()};
}

QImage canonicalImage(const QImage& image, ImageKind) {
  return image;
}

}  // namespace test_support
