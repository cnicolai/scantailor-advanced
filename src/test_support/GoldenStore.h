// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_GOLDENSTORE_H_
#define SCANTAILOR_TEST_SUPPORT_GOLDENSTORE_H_

#include <QImage>
#include <QString>
#include <QStringList>
#include <set>
#include <vector>

#include "ImageCompare.h"
#include "TextGolden.h"

namespace test_support {

/// Whether a freshly produced result may be recorded as a golden (D-05).  `reason` says what is wrong when not.
struct Sanity {
  bool ok;
  QString reason;
};

/// Sanity that always holds, for tests of the store itself and for results with no checkable property.
inline Sanity sanityOk() {
  return Sanity{true, QString()};
}

/**
 * Goldens on disk: <goldenRoot>/<suite>/<name>.png and <name>.txt, one MANIFEST.txt per suite directory.
 * Suite and name must match ^[a-z0-9_]+$ because they become path components.
 *
 * A normal store never writes under goldenRoot.  An update store (UPDATE_GOLDENS=1) writes a golden only when it
 * is missing or fails the comparison and the sanity check passed, and refreshes the suite's MANIFEST.txt once.
 * Every write under goldenRoot goes through QSaveFile.  Failures leave expected/actual/diff files in
 * <artifactRoot>/<suite>/<name>/ (png for images, txt for text).
 */
class GoldenStore {
 public:
  struct Outcome {
    bool pass;
    bool written;
    QString message;
    std::vector<MetricResult> metrics;
    /// Toolchain differences against the suite's MANIFEST.txt; only on the first outcome of a suite per store.
    QStringList warnings;
  };

  GoldenStore(const QString& goldenRoot, const QString& artifactRoot, bool updateMode);

  /// True only when UPDATE_GOLDENS is exactly "1".
  static bool updateRequestedFromEnvironment();

  Outcome checkImage(const QString& suite,
                     const QString& name,
                     const QImage& actual,
                     ImageKind kind,
                     const Sanity& sanity);

  Outcome checkText(const QString& suite, const QString& name, const TextValues& actual, const Sanity& sanity);

 private:
  QString m_goldenRoot;
  QString m_artifactRoot;
  bool m_updateMode;
  std::set<QString> m_handledSuites;  // suites whose manifest was read (normal) or refreshed (update)
};

}  // namespace test_support

#endif  // SCANTAILOR_TEST_SUPPORT_GOLDENSTORE_H_
