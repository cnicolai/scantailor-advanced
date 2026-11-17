// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "GoldenStore.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageWriter>
#include <QRegularExpression>
#include <QSaveFile>
#include <QtGlobal>

#include "Manifest.h"

namespace test_support {

namespace {

const char kManifestFile[] = "MANIFEST.txt";

/// Suite and golden names become path components; anything but lower-case letters, digits and "_" is refused.
bool validComponent(const QString& component) {
  static const QRegularExpression pattern(QRegularExpression::anchoredPattern("[a-z0-9_]+"));
  return pattern.match(component).hasMatch();
}

QString number(double value) {
  return QString::number(value, 'g', 10);
}

QString invalidNameMessage(const QString& suite, const QString& name) {
  return QString("invalid golden location: suite '%1', name '%2' (allowed: lower-case letters, digits and _)")
      .arg(suite, name);
}

/// One line per failing metric, then the artifact folder with the expected, actual and diff images.
QString failureMessage(const QString& suite,
                       const QString& name,
                       const std::vector<MetricResult>& metrics,
                       const QString& artifactFolder) {
  QString message = QString("golden mismatch for %1/%2").arg(suite, name);
  for (const MetricResult& metric : metrics) {
    if (!metric.pass) {
      message += QString("\n  %1 = %2 (limit %3)").arg(metric.metric, number(metric.value), number(metric.limit));
    }
  }
  message += "\n  artifacts: " + artifactFolder;
  return message;
}

QString missingMessage(const QString& suite, const QString& name, const QString& path, const QString& artifactFolder) {
  return QString("no golden for %1/%2 at %3; check the result and record it with UPDATE_GOLDENS=1\n  artifacts: %4")
      .arg(suite, name, path, artifactFolder);
}

/// Atomic: the file ends up with the old or the new content and no temporary file is left behind.
bool saveAtomically(const QString& path, const QByteArray& data, QString* error) {
  if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
    *error = "cannot create directory for " + path;
    return false;
  }
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
    *error = QString("cannot write %1: %2").arg(path, file.errorString());
    file.cancelWriting();
    return false;
  }
  return true;
}

bool saveImageAtomically(const QString& path, const QImage& image, QString* error) {
  if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
    *error = "cannot create directory for " + path;
    return false;
  }
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    *error = QString("cannot write %1: %2").arg(path, file.errorString());
    return false;
  }
  QImageWriter writer(&file, "png");
  if (!writer.write(image) || !file.commit()) {
    *error = QString("cannot write %1: %2").arg(path, writer.errorString());
    file.cancelWriting();
    return false;
  }
  return true;
}

bool allPixelsGrey(const QImage& rgb) {
  for (int y = 0; y < rgb.height(); ++y) {
    const QRgb* line = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));
    for (int x = 0; x < rgb.width(); ++x) {
      if (qRed(line[x]) != qGreen(line[x]) || qGreen(line[x]) != qBlue(line[x])) {
        return false;
      }
    }
  }
  return true;
}

/// What goes into the golden file: the canonical pixels, as 1-bit, 8-bit grey or RGB PNG.
QImage storedImage(const QImage& actual, ImageKind kind) {
  const QImage canonical = canonicalImage(actual, kind);
  if (kind == ImageKind::BILEVEL || !allPixelsGrey(canonical)) {
    return canonical;
  }
  return canonical.convertToFormat(QImage::Format_Grayscale8);
}

QByteArray readBytes(const QString& path, bool* ok) {
  QFile file(path);
  *ok = file.open(QIODevice::ReadOnly);
  return *ok ? file.readAll() : QByteArray();
}

/// Removes the stale artifacts of one golden.  This is the only recursive delete of the store, so it re-checks
/// that the folder lies below the artifact root even though the names were validated before (defence in depth).
void clearArtifacts(const QString& artifactRoot, const QString& folder) {
  const QString root = QDir::cleanPath(artifactRoot);
  const QString clean = QDir::cleanPath(folder);
  if (!clean.startsWith(root + "/")) {
    return;
  }
  if (QFileInfo(clean).isDir()) {
    QDir(clean).removeRecursively();
  }
}

/// Artifacts are diagnostics in the build tree; a failure to write them must not hide the comparison result.
void saveArtifactImage(const QString& folder, const char* file, const QImage& image) {
  if (image.isNull() || !QDir().mkpath(folder)) {
    return;
  }
  QFile::remove(folder + "/" + file);
  image.save(folder + "/" + file, "PNG");
}

void saveArtifactBytes(const QString& folder, const char* file, const QByteArray& data) {
  if (!QDir().mkpath(folder)) {
    return;
  }
  QFile out(folder + "/" + file);
  if (out.open(QIODevice::WriteOnly)) {
    out.write(data);
  }
}

}  // namespace

GoldenStore::GoldenStore(const QString& goldenRoot, const QString& artifactRoot, bool updateMode)
    : m_goldenRoot(goldenRoot), m_artifactRoot(artifactRoot), m_updateMode(updateMode) {}

bool GoldenStore::updateRequestedFromEnvironment() {
  return qgetenv("UPDATE_GOLDENS") == "1";
}

namespace {

/// The state of one suite's MANIFEST.txt handling, shared by images and texts.
class SuiteManifest {
 public:
  SuiteManifest(const QString& goldenRoot, const QString& suite, std::set<QString>& handled)
      : m_path(goldenRoot + "/" + suite + "/" + kManifestFile), m_suite(suite), m_handled(handled) {}

  /// Normal run, first check of the suite: toolchain differences against the recording become warnings only; a
  /// difference never fails.
  void warnOnDifferences(QStringList* warnings) {
    if (!m_handled.insert(m_suite).second) {
      return;
    }
    bool readable = false;
    const QByteArray data = readBytes(m_path, &readable);
    if (!readable) {
      *warnings << "manifest missing: " + m_path;
      return;
    }
    bool parsed = false;
    const TextValues recorded = parseText(data, &parsed);
    if (!parsed) {
      *warnings << "manifest unreadable: " + m_path;
      return;
    }
    *warnings += toolchainDifferences(recorded, currentToolchain());
  }

  /// Update run, first write-worthy check of the suite: records the current toolchain and source commit.
  /// An identical manifest is left alone, so a repeated update changes nothing.  A suite counts as handled only once
  /// its manifest is on disk, so a failed write fails every later golden of the suite instead of only the first.
  bool refresh(QString* error) {
    if (m_handled.count(m_suite)) {
      return true;
    }
    TextValues values = currentToolchain();
    values["source_commit"] = currentSourceCommit();
    const QByteArray data = serializeText(values);
    bool readable = false;
    if (readBytes(m_path, &readable) != data || !readable) {
      if (!saveAtomically(m_path, data, error)) {
        return false;
      }
    }
    m_handled.insert(m_suite);
    return true;
  }

 private:
  QString m_path;
  QString m_suite;
  std::set<QString>& m_handled;
};

}  // namespace

GoldenStore::Outcome GoldenStore::checkImage(const QString& suite,
                                             const QString& name,
                                             const QImage& actual,
                                             ImageKind kind,
                                             const Sanity& sanity) {
  Outcome outcome{false, false, QString(), {}, {}};
  if (!validComponent(suite) || !validComponent(name)) {
    outcome.message = invalidNameMessage(suite, name);
    return outcome;
  }

  const QString path = m_goldenRoot + "/" + suite + "/" + name + ".png";
  const QString folder = m_artifactRoot + "/" + suite + "/" + name;
  SuiteManifest manifest(m_goldenRoot, suite, m_handledSuites);

  QImage expected;
  const bool exists = QFileInfo::exists(path);
  if (exists) {
    expected.load(path);
  }
  CompareResult comparison{false, {}, QImage()};
  if (!expected.isNull()) {
    comparison = compareImages(expected, actual, kind);
  }

  if (!m_updateMode) {
    manifest.warnOnDifferences(&outcome.warnings);
  }

  if (comparison.pass) {
    if (m_updateMode) {
      QString error;
      if (!manifest.refresh(&error)) {
        outcome.message = error;
        return outcome;
      }
    }
    clearArtifacts(m_artifactRoot, folder);
    outcome.pass = true;
    outcome.metrics = comparison.metrics;
    outcome.message = "matches the golden";
    return outcome;
  }

  // Missing, unreadable or failing golden: leave the evidence in the artifact folder.
  clearArtifacts(m_artifactRoot, folder);
  saveArtifactImage(folder, "actual.png", canonicalImage(actual, kind));
  if (!expected.isNull()) {
    saveArtifactImage(folder, "expected.png", canonicalImage(expected, kind));
    saveArtifactImage(folder, "diff.png", comparison.diff);
  }
  outcome.metrics = comparison.metrics;

  QString problem;
  if (!exists) {
    problem = missingMessage(suite, name, path, folder);
  } else if (expected.isNull()) {
    problem = QString("golden %1/%2 cannot be read as an image: %3\n  artifacts: %4").arg(suite, name, path, folder);
  } else {
    problem = failureMessage(suite, name, comparison.metrics, folder);
  }

  if (!m_updateMode) {
    outcome.message = problem;
    return outcome;
  }
  if (!sanity.ok) {
    outcome.message = "sanity failed: " + sanity.reason + "\n" + problem;
    return outcome;
  }

  QString error;
  if (!manifest.refresh(&error) || !saveImageAtomically(path, storedImage(actual, kind), &error)) {
    outcome.message = error;
    return outcome;
  }
  outcome.pass = true;
  outcome.written = true;
  outcome.metrics.clear();  // the metrics belonged to the golden that was just replaced
  outcome.message = exists ? "recorded, replacing a golden that failed:\n" + problem : "recorded " + path;
  return outcome;
}

GoldenStore::Outcome GoldenStore::checkText(const QString& suite,
                                            const QString& name,
                                            const TextValues& actual,
                                            const Sanity& sanity) {
  Outcome outcome{false, false, QString(), {}, {}};
  if (!validComponent(suite) || !validComponent(name)) {
    outcome.message = invalidNameMessage(suite, name);
    return outcome;
  }

  QString offendingKey;
  if (!textRoundTrips(actual, &offendingKey)) {
    outcome.message = QString(
                          "text values for %1/%2 cannot be stored: key '%3' does not survive serializeText and "
                          "parseText (keys must not contain \" = \" or a line break, values no line break)")
                          .arg(suite, name, offendingKey);
    return outcome;
  }

  const QString path = m_goldenRoot + "/" + suite + "/" + name + ".txt";
  const QString folder = m_artifactRoot + "/" + suite + "/" + name;
  SuiteManifest manifest(m_goldenRoot, suite, m_handledSuites);

  bool readable = false;
  const QByteArray goldenBytes = readBytes(path, &readable);
  bool parsed = false;
  TextValues expected;
  CompareResult comparison{false, {}, QImage()};
  if (readable) {
    expected = parseText(goldenBytes, &parsed);
    if (parsed) {
      comparison = compareText(expected, actual);
    }
  }

  if (!m_updateMode) {
    manifest.warnOnDifferences(&outcome.warnings);
  }

  if (comparison.pass) {
    if (m_updateMode) {
      QString error;
      if (!manifest.refresh(&error)) {
        outcome.message = error;
        return outcome;
      }
    }
    clearArtifacts(m_artifactRoot, folder);
    outcome.pass = true;
    outcome.metrics = comparison.metrics;
    outcome.message = "matches the golden";
    return outcome;
  }

  clearArtifacts(m_artifactRoot, folder);
  saveArtifactBytes(folder, "actual.txt", serializeText(actual));
  if (parsed) {
    saveArtifactBytes(folder, "expected.txt", goldenBytes);
    saveArtifactBytes(folder, "diff.txt", textDiff(expected, actual));
  }
  outcome.metrics = comparison.metrics;

  QString problem;
  if (!readable) {
    problem = missingMessage(suite, name, path, folder);
  } else if (!parsed) {
    problem = QString("golden %1/%2 cannot be parsed: %3\n  artifacts: %4").arg(suite, name, path, folder);
  } else {
    problem = failureMessage(suite, name, comparison.metrics, folder);
  }

  if (!m_updateMode) {
    outcome.message = problem;
    return outcome;
  }
  if (!sanity.ok) {
    outcome.message = "sanity failed: " + sanity.reason + "\n" + problem;
    return outcome;
  }

  QString error;
  if (!manifest.refresh(&error) || !saveAtomically(path, serializeText(actual), &error)) {
    outcome.message = error;
    return outcome;
  }
  outcome.pass = true;
  outcome.written = true;
  outcome.metrics.clear();
  outcome.message = readable ? "recorded, replacing a golden that failed:\n" + problem : "recorded " + path;
  return outcome;
}

}  // namespace test_support
