// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "GoldenStore.h"

namespace test_support {

// Interface stubs for the RED commit of plan 02-02 task 3: the tests compile and fail on their assertions.
GoldenStore::GoldenStore(const QString& goldenRoot, const QString& artifactRoot, bool updateMode)
    : m_goldenRoot(goldenRoot), m_artifactRoot(artifactRoot), m_updateMode(updateMode) {}

bool GoldenStore::updateRequestedFromEnvironment() {
  return false;
}

GoldenStore::Outcome GoldenStore::checkImage(const QString&,
                                             const QString&,
                                             const QImage&,
                                             ImageKind,
                                             const Sanity&) {
  return Outcome{false, false, QString(), {}, {}};
}

GoldenStore::Outcome GoldenStore::checkText(const QString&, const QString&, const TextValues&, const Sanity&) {
  return Outcome{false, false, QString(), {}, {}};
}

}  // namespace test_support
