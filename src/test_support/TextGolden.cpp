// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "TextGolden.h"

namespace test_support {

// Interface stubs for the RED commit of plan 02-02 task 2: the tests compile and fail on their assertions.
QString formatNumber(double, int) {
  return QString();
}

QByteArray serializeText(const TextValues&) {
  return QByteArray();
}

TextValues parseText(const QByteArray&, bool* ok) {
  if (ok) {
    *ok = true;
  }
  return TextValues();
}

CompareResult compareText(const TextValues&, const TextValues&) {
  return CompareResult{false, {}, QImage()};
}

QByteArray textDiff(const TextValues&, const TextValues&) {
  return QByteArray();
}

}  // namespace test_support
