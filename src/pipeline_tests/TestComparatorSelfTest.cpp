// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <GoldenStore.h>
#include <ImageCompare.h>
#include <Rng.h>
#include <Tolerances.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QRgb>
#include <QString>
#include <QTemporaryDir>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <cstdint>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// D-13: the comparator limits are frozen from measurement (Tolerances.h), and this suite proves on the recorded
// goldens that they still catch what a regression looks like: isolated specks, a one pixel shift and a small
// grey-level offset.  The store is built with update mode off, so a run with UPDATE_GOLDENS=1 cannot write a
// golden from here (D-06).  Artifacts go to a temporary root under the build tree.

namespace test_support {
namespace tests {

namespace {

constexpr uint64_t kSpeckSeed = 0x5EED0300ULL;
constexpr int kSpeckCount = 300;
constexpr int kSpeckClearance = 3;  // pixels of paper around every speck, in every direction
constexpr int kLevelOffset = 10;
// Largest share of the pixels the single-level case may touch: 10 levels on 0.2% of the pixels is a mean error of 0.02,
// under the frozen mae limit, so only max_abs_error can catch it.
constexpr double kLocalLevelShare = 0.002;

QString goldenDir() {
  return QString::fromUtf8(GOLDEN_DIR);
}

/// Sets an environment variable for the lifetime of the object.  The destructor restores the previous value (or unsets
/// the variable) on every exit, including the exception a failed BOOST_REQUIRE throws.
class ScopedEnvironmentVariable {
 public:
  ScopedEnvironmentVariable(const char* name, const QByteArray& value)
      : m_name(name), m_wasSet(qEnvironmentVariableIsSet(name)), m_previous(qgetenv(name)) {
    qputenv(name, value);
  }

  ScopedEnvironmentVariable(const ScopedEnvironmentVariable&) = delete;
  ScopedEnvironmentVariable& operator=(const ScopedEnvironmentVariable&) = delete;

  ~ScopedEnvironmentVariable() {
    if (m_wasSet) {
      qputenv(m_name.constData(), m_previous);
    } else {
      qunsetenv(m_name.constData());
    }
  }

 private:
  QByteArray m_name;
  bool m_wasSet;
  QByteArray m_previous;
};

struct Fingerprint {
  qint64 size;
  QByteArray sha256;

  bool operator==(const Fingerprint& other) const { return size == other.size && sha256 == other.sha256; }
};

using Snapshot = std::map<QString, Fingerprint>;

/// Relative name, size and SHA-256 of every file under the golden directory.
Snapshot snapshotGoldenDirectory() {
  Snapshot snapshot;
  const QDir root(goldenDir());
  QDirIterator it(goldenDir(), QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QString path = it.next();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
      snapshot[root.relativeFilePath(path)] = Fingerprint{-1, QByteArray()};
      continue;
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(&file);
    snapshot[root.relativeFilePath(path)] = Fingerprint{file.size(), hash.result().toHex()};
  }
  return snapshot;
}

std::string describeDifference(const Snapshot& before, const Snapshot& after) {
  std::string text;
  for (const auto& entry : before) {
    const auto found = after.find(entry.first);
    if (found == after.end()) {
      text += " removed: " + entry.first.toStdString();
    } else if (!(found->second == entry.second)) {
      text += " changed: " + entry.first.toStdString();
    }
  }
  for (const auto& entry : after) {
    if (before.find(entry.first) == before.end()) {
      text += " added: " + entry.first.toStdString();
    }
  }
  return text;
}

/// Every case leaves the golden directory as it found it.
struct GoldenDirectoryGuard {
  GoldenDirectoryGuard() : before(snapshotGoldenDirectory()) {
    BOOST_REQUIRE_MESSAGE(!before.empty(), "no files under " << GOLDEN_DIR);
  }

  ~GoldenDirectoryGuard() {
    const Snapshot after = snapshotGoldenDirectory();
    BOOST_CHECK_MESSAGE(after == before, "golden directory modified:" << describeDifference(before, after));
  }

  Snapshot before;
};

/// Artifact root of one comparison, inside the build tree's temporary directory.
struct ArtifactRoot {
  ArtifactRoot() : dir(QString::fromUtf8(TEST_TMP_ROOT) + "/comparator_self_test-XXXXXX") {
    BOOST_REQUIRE_MESSAGE(dir.isValid(), "cannot create a temporary directory under " << TEST_TMP_ROOT);
  }

  QString path() const { return dir.path(); }

  QTemporaryDir dir;
};

QImage loadGolden(const char* suite, const char* name) {
  const QString path = goldenDir() + "/" + suite + "/" + name + ".png";
  const QImage image(path);
  BOOST_REQUIRE_MESSAGE(!image.isNull(), "cannot read golden " << path.toStdString());
  return image;
}

/// The one way this suite compares: the real golden directory, update mode hard-wired off.
GoldenStore::Outcome compareWithGolden(const QString& suite,
                                       const QString& name,
                                       const QImage& actual,
                                       ImageKind kind,
                                       const ArtifactRoot& artifacts) {
  GoldenStore store(goldenDir(), artifacts.path(), false);
  return store.checkImage(suite, name, actual, kind, sanityOk());
}

const MetricResult* findMetric(const GoldenStore::Outcome& outcome, const QString& name) {
  for (const MetricResult& metric : outcome.metrics) {
    if (metric.metric == name) {
      return &metric;
    }
  }
  return nullptr;
}

std::string describe(const GoldenStore::Outcome& outcome) {
  std::string text;
  for (const MetricResult& metric : outcome.metrics) {
    text += " " + metric.metric.toStdString() + "=" + std::to_string(metric.value) + " (limit "
            + std::to_string(metric.limit) + (metric.pass ? ", pass)" : ", FAIL)");
  }
  return text;
}

void requireMetricFails(const GoldenStore::Outcome& outcome, const char* name) {
  const MetricResult* metric = findMetric(outcome, name);
  BOOST_REQUIRE_MESSAGE(metric != nullptr, "metric " << name << " missing:" << describe(outcome));
  BOOST_CHECK_MESSAGE(!metric->pass, "metric " << name << " should fail:" << describe(outcome));
}

/// Greyscale copy with 0 = ink and 255 = paper, whatever the golden's own format.
QImage toGrey(const QImage& golden) {
  return golden.convertToFormat(QImage::Format_Grayscale8);
}

bool paperAround(const QImage& grey, int x, int y, int radius) {
  for (int yy = y - radius; yy <= y + radius; ++yy) {
    const uint8_t* line = grey.constScanLine(yy);
    for (int xx = x - radius; xx <= x + radius; ++xx) {
      if (line[xx] < 128) {
        return false;
      }
    }
  }
  return true;
}

/// Sets `count` paper pixels to ink; every one lies at least kSpeckClearance pixels from any ink pixel and from the
/// other specks, so each becomes a component of its own.
QImage withIsolatedSpecks(const QImage& golden, int count, uint64_t seed) {
  QImage grey = toGrey(golden);
  const int margin = kSpeckClearance;
  BOOST_REQUIRE(grey.width() > 2 * margin + 1 && grey.height() > 2 * margin + 1);
  Rng rng(seed);
  int placed = 0;
  for (int attempts = 0; placed < count; ++attempts) {
    BOOST_REQUIRE_MESSAGE(attempts < 1000000, "only " << placed << " of " << count << " specks fit");
    const int x = margin + int(rng.below(uint32_t(grey.width() - 2 * margin)));
    const int y = margin + int(rng.below(uint32_t(grey.height() - 2 * margin)));
    if (paperAround(grey, x, y, kSpeckClearance)) {
      grey.scanLine(y)[x] = 0;
      ++placed;
    }
  }
  return grey;
}

/// Moves the content by (dx, dy) pixels; the uncovered strip is filled with `fill` (the raw pixel value).
QImage shifted(const QImage& image, int dx, int dy, uint32_t fill) {
  const bool grey = image.format() == QImage::Format_Grayscale8;
  BOOST_REQUIRE(grey || image.format() == QImage::Format_RGB32);
  const int bytes = grey ? 1 : 4;
  QImage out(image.size(), image.format());
  for (int y = 0; y < out.height(); ++y) {
    uint8_t* dst = out.scanLine(y);
    for (int x = 0; x < out.width(); ++x) {
      std::memcpy(dst + x * bytes, &fill, size_t(bytes));
    }
  }
  for (int y = 0; y < image.height(); ++y) {
    const int ty = y + dy;
    if (ty < 0 || ty >= image.height()) {
      continue;
    }
    const int from = std::max(0, -dx);
    const int to = std::min(image.width(), image.width() - dx);
    if (to > from) {
      std::memcpy(out.scanLine(ty) + (from + dx) * bytes, image.constScanLine(y) + from * bytes,
                  size_t(to - from) * size_t(bytes));
    }
  }
  return out;
}

int clampLevel(int value) {
  return std::min(255, value);
}

/// Raises every channel by `delta` (clamped at 255) on all pixels, or only on pixels whose grey level is
/// `onlyLevel` when that is not negative.
QImage raisedLevels(const QImage& golden, int delta, int onlyLevel) {
  QImage out = golden.convertToFormat(QImage::Format_RGB32);
  for (int y = 0; y < out.height(); ++y) {
    QRgb* line = reinterpret_cast<QRgb*>(out.scanLine(y));
    for (int x = 0; x < out.width(); ++x) {
      if (onlyLevel >= 0 && qGray(line[x]) != onlyLevel) {
        continue;
      }
      line[x] = qRgb(clampLevel(qRed(line[x]) + delta), clampLevel(qGreen(line[x]) + delta),
                     clampLevel(qBlue(line[x]) + delta));
    }
  }
  return out;
}

/// The most frequent grey level that `delta` can raise without clamping and that covers at most `maxShare` of the
/// pixels: a local change that the mean error stays below its limit for.
int mostFrequentLocalLevel(const QImage& golden, int delta, double maxShare) {
  const QImage rgb = golden.convertToFormat(QImage::Format_RGB32);
  std::vector<qint64> histogram(256, 0);
  for (int y = 0; y < rgb.height(); ++y) {
    const QRgb* line = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));
    for (int x = 0; x < rgb.width(); ++x) {
      ++histogram[size_t(qGray(line[x]))];
    }
  }
  const double limit = maxShare * double(rgb.width()) * double(rgb.height());
  int best = -1;
  for (int level = 0; level + delta <= 255; ++level) {
    const qint64 count = histogram[size_t(level)];
    if (count > 0 && double(count) <= limit && (best < 0 || count > histogram[size_t(best)])) {
      best = level;
    }
  }
  BOOST_REQUIRE_MESSAGE(best >= 0, "no grey level covers at most " << maxShare << " of the pixels");
  return best;
}

/// One altered copy of a recorded golden and the metrics that must catch it.
struct Alteration {
  std::string label;
  QString suite;
  QString name;
  ImageKind kind;
  QImage altered;
  std::vector<QString> failingMetrics;
};

Alteration flippedPaperPixels() {
  const QImage golden = loadGolden("flat", "flat_600_bw");
  return Alteration{"300 isolated paper pixels set to ink",
                    "flat",
                    "flat_600_bw",
                    ImageKind::BILEVEL,
                    withIsolatedSpecks(golden, kSpeckCount, kSpeckSeed),
                    {"component_count_relative"}};
}

Alteration oneXShiftBilevel() {
  const QImage golden = loadGolden("flat", "flat_600_bw");
  return Alteration{"bi-level shifted right by 1 px",   "flat",   "flat_600_bw", ImageKind::BILEVEL,
                    shifted(toGrey(golden), 1, 0, 255), {"shift"}};
}

Alteration greyOffsetAllPixels() {
  const QImage golden = loadGolden("flat", "flat_300_color");
  return Alteration{
      "every channel raised by 10", "flat", "flat_300_color", ImageKind::COLOR, raisedLevels(golden, kLevelOffset, -1),
      {"mae", "max_abs_error"}};
}

Alteration greyOffsetOneLevel() {
  const QImage golden = loadGolden("flat", "flat_300_color");
  const int level = mostFrequentLocalLevel(golden, kLevelOffset, kLocalLevelShare);
  return Alteration{"one grey level raised by 10",
                    "flat",
                    "flat_300_color",
                    ImageKind::COLOR,
                    raisedLevels(golden, kLevelOffset, level),
                    {"max_abs_error"}};
}

Alteration oneYShiftColor() {
  const QImage golden = loadGolden("flat", "flat_300_color");
  return Alteration{"colour shifted down by 1 px",
                    "flat",
                    "flat_300_color",
                    ImageKind::COLOR,
                    shifted(golden.convertToFormat(QImage::Format_RGB32), 0, 1, 0xFFFFFFFFu),
                    {"shift"}};
}

Alteration mixedOutputOffset() {
  const QImage golden = loadGolden("mixed", "mixed_300_mixed");
  return Alteration{"mixed output, every channel raised by 10", "mixed",          "mixed_300_mixed", ImageKind::COLOR,
                    raisedLevels(golden, kLevelOffset, -1),     {"max_abs_error"}};
}

std::vector<Alteration> allAlterations() {
  std::vector<Alteration> all;
  all.push_back(flippedPaperPixels());
  all.push_back(oneXShiftBilevel());
  all.push_back(greyOffsetAllPixels());
  all.push_back(greyOffsetOneLevel());
  all.push_back(oneYShiftColor());
  all.push_back(mixedOutputOffset());
  return all;
}

/// The altered copy must fail, and the metrics the case names must be among the failing ones.
GoldenStore::Outcome requireFailure(const Alteration& alteration, const ArtifactRoot& artifacts) {
  const GoldenStore::Outcome outcome
      = compareWithGolden(alteration.suite, alteration.name, alteration.altered, alteration.kind, artifacts);
  BOOST_TEST_MESSAGE("SELFTEST " << alteration.label << ":" << describe(outcome));
  BOOST_CHECK_MESSAGE(!outcome.pass, alteration.label << " passed the comparison:" << describe(outcome));
  for (const QString& metric : alteration.failingMetrics) {
    requireMetricFails(outcome, metric.toUtf8().constData());
  }
  return outcome;
}

QString artifactFolder(const Alteration& alteration, const ArtifactRoot& artifacts) {
  return artifacts.path() + "/" + alteration.suite + "/" + alteration.name;
}

}  // namespace

BOOST_FIXTURE_TEST_SUITE(ComparatorSelfTestSuite, GoldenDirectoryGuard)

BOOST_AUTO_TEST_CASE(test_unaltered_goldens_pass) {
  struct Subject {
    const char* suite;
    const char* name;
    ImageKind kind;
  };
  const Subject subjects[] = {{"flat", "flat_600_bw", ImageKind::BILEVEL},
                              {"flat", "flat_300_color", ImageKind::COLOR},
                              {"mixed", "mixed_300_mixed", ImageKind::COLOR}};
  for (const Subject& subject : subjects) {
    const ArtifactRoot artifacts;
    const QImage golden = loadGolden(subject.suite, subject.name);
    const GoldenStore::Outcome outcome
        = compareWithGolden(subject.suite, subject.name, golden, subject.kind, artifacts);
    BOOST_CHECK_MESSAGE(outcome.pass, subject.suite << "/" << subject.name
                                                    << " fails against itself: " << outcome.message.toStdString());
    for (const MetricResult& metric : outcome.metrics) {
      BOOST_CHECK_MESSAGE(metric.value == 0.0, subject.name << ": " << metric.metric.toStdString() << " = "
                                                            << metric.value << " for an identical image");
    }
    BOOST_CHECK_MESSAGE(!QFileInfo::exists(artifacts.path() + "/" + subject.suite + "/" + subject.name),
                        "a passing comparison left an artifact folder");
  }
}

BOOST_AUTO_TEST_CASE(test_flipped_paper_pixels_fail) {
  const ArtifactRoot artifacts;
  const Alteration alteration = flippedPaperPixels();
  requireFailure(alteration, artifacts);
}

BOOST_AUTO_TEST_CASE(test_one_pixel_shift_bilevel_fails) {
  const ArtifactRoot artifacts;
  const Alteration alteration = oneXShiftBilevel();
  const GoldenStore::Outcome outcome = requireFailure(alteration, artifacts);
  const MetricResult* shift = findMetric(outcome, "shift");
  BOOST_REQUIRE(shift != nullptr);
  BOOST_CHECK_EQUAL(shift->limit, 0.0);
}

BOOST_AUTO_TEST_CASE(test_grey_offset_fails) {
  const ArtifactRoot artifacts;
  const Alteration alteration = greyOffsetAllPixels();
  const GoldenStore::Outcome outcome = requireFailure(alteration, artifacts);
  const MetricResult* mae = findMetric(outcome, "mae");
  const MetricResult* maxError = findMetric(outcome, "max_abs_error");
  BOOST_REQUIRE(mae != nullptr && maxError != nullptr);
  // The frozen limits themselves, not an override.
  BOOST_CHECK_EQUAL(mae->limit, tolerance::kColorMeanAbsError);
  BOOST_CHECK_EQUAL(maxError->limit, tolerance::kColorMaxAbsError);
  BOOST_CHECK_EQUAL(maxError->value, double(kLevelOffset));
}

BOOST_AUTO_TEST_CASE(test_single_grey_level_offset_fails) {
  const ArtifactRoot artifacts;
  const Alteration alteration = greyOffsetOneLevel();
  const GoldenStore::Outcome outcome = requireFailure(alteration, artifacts);
  const MetricResult* maxError = findMetric(outcome, "max_abs_error");
  BOOST_REQUIRE(maxError != nullptr);
  BOOST_CHECK_EQUAL(maxError->value, double(kLevelOffset));
  // The change is local: the mean error stays under its limit and max_abs_error alone catches it.
  const MetricResult* mae = findMetric(outcome, "mae");
  BOOST_REQUIRE(mae != nullptr);
  BOOST_CHECK_MESSAGE(mae->pass, "mae should not see a local change:" << describe(outcome));
}

BOOST_AUTO_TEST_CASE(test_one_pixel_shift_color_fails) {
  const ArtifactRoot artifacts;
  const Alteration alteration = oneYShiftColor();
  requireFailure(alteration, artifacts);
}

BOOST_AUTO_TEST_CASE(test_mixed_output_offset_fails) {
  const ArtifactRoot artifacts;
  const Alteration alteration = mixedOutputOffset();
  requireFailure(alteration, artifacts);
}

BOOST_AUTO_TEST_CASE(test_artifacts_written_for_each_failure) {
  for (const Alteration& alteration : allAlterations()) {
    const ArtifactRoot artifacts;
    const GoldenStore::Outcome outcome = requireFailure(alteration, artifacts);
    const QString folder = artifactFolder(alteration, artifacts);

    const QImage golden = loadGolden(alteration.suite.toUtf8().constData(), alteration.name.toUtf8().constData());
    for (const char* file : {"expected.png", "actual.png", "diff.png"}) {
      const QImage artifact(folder + "/" + file);
      BOOST_CHECK_MESSAGE(!artifact.isNull(),
                          alteration.label << ": " << file << " missing in " << folder.toStdString());
      BOOST_CHECK_MESSAGE(artifact.size() == golden.size(), alteration.label << ": " << file << " has size "
                                                                             << artifact.width() << "x"
                                                                             << artifact.height());
    }

    // The message names every failing metric with its value and limit, and the folder.
    bool anyFailing = false;
    for (const MetricResult& metric : outcome.metrics) {
      if (metric.pass) {
        continue;
      }
      anyFailing = true;
      const QString line
          = QString("%1 = %2 (limit %3)")
                .arg(metric.metric, QString::number(metric.value, 'g', 10), QString::number(metric.limit, 'g', 10));
      BOOST_CHECK_MESSAGE(outcome.message.contains(line), alteration.label << ": message lacks '" << line.toStdString()
                                                                           << "': " << outcome.message.toStdString());
    }
    BOOST_CHECK_MESSAGE(anyFailing, alteration.label << ": no failing metric reported");
    BOOST_CHECK_MESSAGE(outcome.message.contains(folder),
                        alteration.label << ": message lacks the folder " << folder.toStdString());
  }
}

// WR-01: a failing BOOST_REQUIRE throws; the update flag must not stay set for the suites that run after it.
BOOST_AUTO_TEST_CASE(test_update_flag_restored_when_an_assertion_throws) {
  const ScopedEnvironmentVariable outer("UPDATE_GOLDENS", "0");  // keeps the process state of the case

  const auto setInsideAThrowingScope = []() {
    try {
      const ScopedEnvironmentVariable inner("UPDATE_GOLDENS", "1");
      BOOST_CHECK(qgetenv("UPDATE_GOLDENS") == "1");
      BOOST_CHECK(GoldenStore::updateRequestedFromEnvironment());
      throw std::runtime_error("stand-in for the exception of a failed BOOST_REQUIRE");
    } catch (const std::runtime_error&) {
    }
  };

  setInsideAThrowingScope();
  BOOST_CHECK(qEnvironmentVariableIsSet("UPDATE_GOLDENS"));
  BOOST_CHECK(qgetenv("UPDATE_GOLDENS") == "0");
  BOOST_CHECK(!GoldenStore::updateRequestedFromEnvironment());

  qunsetenv("UPDATE_GOLDENS");
  setInsideAThrowingScope();
  BOOST_CHECK_MESSAGE(!qEnvironmentVariableIsSet("UPDATE_GOLDENS"), "the variable was left set after the throw");
  BOOST_CHECK(!GoldenStore::updateRequestedFromEnvironment());
}

BOOST_AUTO_TEST_CASE(test_golden_directory_untouched) {
  // Whatever UPDATE_GOLDENS says for the process, this suite's store is built with update mode off.
  const ScopedEnvironmentVariable updateRequested("UPDATE_GOLDENS", "1");

  const Snapshot before = snapshotGoldenDirectory();
  for (const Alteration& alteration : allAlterations()) {
    const ArtifactRoot artifacts;
    const GoldenStore::Outcome outcome = requireFailure(alteration, artifacts);
    BOOST_CHECK_MESSAGE(!outcome.written, alteration.label << ": the self-test store wrote a golden");
  }
  const QImage golden = loadGolden("flat", "flat_600_bw");
  const ArtifactRoot artifacts;
  const GoldenStore::Outcome unaltered
      = compareWithGolden("flat", "flat_600_bw", golden, ImageKind::BILEVEL, artifacts);
  BOOST_CHECK(unaltered.pass);
  BOOST_CHECK(!unaltered.written);
  const Snapshot after = snapshotGoldenDirectory();

  BOOST_CHECK_MESSAGE(after == before, "golden directory modified:" << describeDifference(before, after));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace tests
}  // namespace test_support
