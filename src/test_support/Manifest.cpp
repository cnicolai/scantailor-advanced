// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Manifest.h"

namespace test_support {

// Interface stubs for the RED commit of plan 02-02 task 2: the tests compile and fail on their assertions.
TextValues currentToolchain() {
  return TextValues();
}

QString currentSourceCommit() {
  return QString();
}

QStringList toolchainDifferences(const TextValues&, const TextValues&) {
  return QStringList();
}

}  // namespace test_support
