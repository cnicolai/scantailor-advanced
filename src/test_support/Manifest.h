// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_MANIFEST_H_
#define SCANTAILOR_TEST_SUPPORT_MANIFEST_H_

#include <QString>
#include <QStringList>

#include "TextGolden.h"

namespace test_support {

/// Versions of the toolchain this binary was built with and runs on: qt_runtime, qt_compile, boost, libtiff, libpng,
/// libjpeg_turbo, apple_clang, xcode, cmake, macos, ffp_contract and build_type.  No source_commit.
TextValues currentToolchain();

/// `git rev-parse HEAD` in the source directory; "unknown" on any failure.
QString currentSourceCommit();

/// One entry per key whose recorded value differs from the current one, naming the key and both values.  source_commit
/// and build_type are ignored: they change legitimately between a recording and a check.
QStringList toolchainDifferences(const TextValues& recorded, const TextValues& current);

}  // namespace test_support

#endif  // SCANTAILOR_TEST_SUPPORT_MANIFEST_H_
