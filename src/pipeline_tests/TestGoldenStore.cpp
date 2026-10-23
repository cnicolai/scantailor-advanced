// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ExpectedFailure.h>
#include <GoldenStore.h>
#include <Manifest.h>
#include <TextGolden.h>
#include <Tolerances.h>

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QLocale>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QThread>
#include <boost/test/unit_test.hpp>
#include <map>
#include <sstream>
#include <string>

namespace test_support {
namespace tests {

namespace {

const MetricResult* findMetric(const CompareResult& result, const QString& name) {
  for (const MetricResult& m : result.metrics) {
    if (m.metric == name) {
      return &m;
    }
  }
  return nullptr;
}

std::string describe(const CompareResult& result) {
  std::ostringstream out;
  for (const MetricResult& m : result.metrics) {
    out << m.metric.toStdString() << "=" << m.value << " (limit " << m.limit << ", " << (m.pass ? "pass" : "fail")
        << ") ";
  }
  return out.str();
}

/// One key with the given values on both sides.
CompareResult compareOne(const QString& key, const QString& expected, const QString& actual) {
  return compareText(TextValues{{key, expected}}, TextValues{{key, actual}});
}

/// Makes the process locale one that formats 1.5 as "1,5" for the lifetime of the object.
class GermanLocale {
 public:
  GermanLocale() : m_previous(QLocale()) { QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany)); }

  ~GermanLocale() { QLocale::setDefault(m_previous); }

 private:
  QLocale m_previous;
};


/// Golden and artifact roots of one test, both in the build tree's temporary directory and never the real golden
/// directory.
struct Roots {
  Roots() { BOOST_REQUIRE(golden.isValid() && artifacts.isValid()); }

  QString goldenPath() const { return golden.path(); }

  QString artifactPath() const { return artifacts.path(); }

  QTemporaryDir golden;
  QTemporaryDir artifacts;
};

struct FileState {
  bool isDirectory;
  QByteArray content;
  QDateTime modified;

  bool operator==(const FileState& other) const {
    return isDirectory == other.isDirectory && content == other.content && modified == other.modified;
  }

  bool operator!=(const FileState& other) const { return !(*this == other); }
};

using Snapshot = std::map<QString, FileState>;

/// Every file and directory below `root` with content and modification time.
Snapshot snapshotOf(const QString& root) {
  Snapshot snapshot;
  QDirIterator it(root, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QString path = it.next();
    const QFileInfo info(path);
    FileState state{info.isDir(), QByteArray(), info.lastModified()};
    if (!info.isDir()) {
      QFile file(path);
      BOOST_REQUIRE(file.open(QIODevice::ReadOnly));
      state.content = file.readAll();
    }
    snapshot[QDir(root).relativeFilePath(path)] = state;
  }
  return snapshot;
}

/// White page with two ink blocks; variant 1 adds a third block, which is far above any edge tolerance.
QImage makePage(int variant) {
  QImage img(120, 80, QImage::Format_Grayscale8);
  img.fill(255);
  const auto block = [&img](int x0, int y0, int w, int h) {
    for (int y = y0; y < y0 + h; ++y) {
      for (int x = x0; x < x0 + w; ++x) {
        img.scanLine(y)[x] = 0;
      }
    }
  };
  block(10, 10, 30, 40);
  block(60, 20, 40, 10);
  if (variant == 1) {
    block(20, 60, 40, 15);
  }
  return img;
}

GoldenStore::Outcome checkPage(GoldenStore& store,
                               const QString& suite,
                               const QString& name,
                               int variant,
                               const Sanity& sanity = sanityOk()) {
  return store.checkImage(suite, name, makePage(variant), ImageKind::BILEVEL, sanity);
}

GoldenStore::Outcome recordImage(const Roots& roots, const QString& suite, const QString& name, int variant) {
  GoldenStore store(roots.goldenPath(), roots.artifactPath(), true);
  const GoldenStore::Outcome outcome = store.checkImage(suite, name, makePage(variant), ImageKind::BILEVEL, sanityOk());
  BOOST_REQUIRE_MESSAGE(outcome.pass && outcome.written, outcome.message.toStdString());
  return outcome;
}

QString readFile(const QString& path) {
  QFile file(path);
  BOOST_REQUIRE_MESSAGE(file.open(QIODevice::ReadOnly), "cannot open " << path.toStdString());
  return QString::fromUtf8(file.readAll());
}

void writeFile(const QString& path, const QByteArray& data) {
  QFile file(path);
  BOOST_REQUIRE(file.open(QIODevice::WriteOnly));
  file.write(data);
}

}  // namespace

BOOST_AUTO_TEST_SUITE(GoldenStoreTestSuite)

BOOST_AUTO_TEST_CASE(test_text_roundtrip_and_parse_errors) {
  const TextValues values{{"b", "2"}, {"a", "1"}};
  const QByteArray data = serializeText(values);
  BOOST_CHECK_EQUAL(data.toStdString(), "a = 1\nb = 2\n");

  bool ok = false;
  const TextValues parsed = parseText(data, &ok);
  BOOST_CHECK(ok);
  BOOST_CHECK(parsed == values);

  // A value may itself contain " = " and may be empty after the separator.
  ok = false;
  const TextValues odd = parseText("expr = x = y\n", &ok);
  BOOST_CHECK(ok);
  BOOST_REQUIRE_EQUAL(odd.size(), 1u);
  BOOST_CHECK_EQUAL(odd.at("expr").toStdString(), "x = y");

  ok = true;
  BOOST_CHECK(parseText("a = 1\nno separator here\n", &ok).empty());
  BOOST_CHECK(!ok);

  ok = true;
  BOOST_CHECK(parseText("a = 1\na = 2\n", &ok).empty());
  BOOST_CHECK_MESSAGE(!ok, "a repeated key must be rejected");

  ok = true;
  BOOST_CHECK(parseText(" = 1\n", &ok).empty());
  BOOST_CHECK_MESSAGE(!ok, "an empty key must be rejected");

  BOOST_CHECK(parseText("", nullptr).empty());  // a null ok pointer is allowed

  const GermanLocale german;
  BOOST_CHECK_EQUAL(formatNumber(1.5).toStdString(), "1.5000");
  BOOST_CHECK_EQUAL(formatNumber(2.0, 2).toStdString(), "2.00");
  BOOST_CHECK_EQUAL(formatNumber(-0.00001).toStdString(), "0.0000");
  BOOST_CHECK_EQUAL(formatNumber(-1.26, 1).toStdString(), "-1.3");
}

BOOST_AUTO_TEST_CASE(test_text_tolerances_by_suffix) {
  const struct {
    const char* key;
    const char* expected;
    const char* actual;
    bool pass;
  } cases[] = {
      {"angle_deg", "0.0000", "0.0090", true},
      {"angle_deg", "0.0000", "0.0200", false},
      {"x_px", "10.0", "11.0", true},
      {"x_px", "10.0", "11.5", false},
      {"width_mm", "10.0000", "10.0500", true},
      {"width_mm", "10.0000", "10.1000", false},
      {"output_dpi", "600", "601", false},
      {"output_dpi", "600", "600", true},
      {"sub_page", "left", "right", false},
      {"x_px", "10.0", "wide", false},  // non-numeric value under a numeric suffix
  };
  for (const auto& c : cases) {
    const CompareResult result = compareOne(c.key, c.expected, c.actual);
    BOOST_CHECK_MESSAGE(result.pass == c.pass,
                        c.key << " " << c.expected << " vs " << c.actual << ": " << describe(result));
    BOOST_CHECK_MESSAGE(findMetric(result, QString("text:") + c.key) != nullptr, "metric text:" << c.key << " missing");
  }

  const CompareResult exact = compareOne("angle_deg", "0.0000", "0.0090");
  const MetricResult* metric = findMetric(exact, "text:angle_deg");
  BOOST_REQUIRE(metric != nullptr);
  BOOST_CHECK_EQUAL(metric->limit, tolerance::kGeometryAngleDeg);

  // A key present on one side only fails with key:<name>, whichever side lacks it.
  const CompareResult missing = compareText(TextValues{{"a", "1"}, {"b", "2"}}, TextValues{{"a", "1"}});
  BOOST_CHECK(!missing.pass);
  const MetricResult* keyMetric = findMetric(missing, "key:b");
  BOOST_REQUIRE_MESSAGE(keyMetric != nullptr, describe(missing));
  BOOST_CHECK(!keyMetric->pass);
  const CompareResult extra = compareText(TextValues{{"a", "1"}}, TextValues{{"a", "1"}, {"c", "3"}});
  BOOST_CHECK(!extra.pass);
  BOOST_CHECK_MESSAGE(findMetric(extra, "key:c") != nullptr, describe(extra));

  BOOST_CHECK(compareText(TextValues{{"a", "1"}}, TextValues{{"a", "1"}}).pass);
}

BOOST_AUTO_TEST_CASE(test_manifest_has_all_keys) {
  const TextValues toolchain = currentToolchain();
  for (const char* key : {"qt_runtime", "qt_compile", "boost", "libtiff", "libpng", "libjpeg_turbo", "apple_clang",
                          "xcode", "cmake", "macos", "ffp_contract", "build_type"}) {
    const auto it = toolchain.find(key);
    BOOST_REQUIRE_MESSAGE(it != toolchain.end(), "manifest key missing: " << key);
    BOOST_CHECK_MESSAGE(!it->second.isEmpty(), "manifest value empty: " << key);
  }
  BOOST_CHECK_MESSAGE(toolchain.find("source_commit") == toolchain.end(),
                      "currentToolchain() must not hold source_commit");
  BOOST_CHECK_EQUAL(toolchain.at("ffp_contract").toStdString(), "off");
  BOOST_CHECK_EQUAL(toolchain.at("qt_runtime").toStdString(), std::string(qVersion()));

  const QString commit = currentSourceCommit();
  const QRegularExpression commitPattern(QRegularExpression::anchoredPattern("[0-9a-f]{40}|unknown"));
  BOOST_CHECK_MESSAGE(commitPattern.match(commit).hasMatch(),
                      "source commit is neither 40 hex digits nor 'unknown': " << commit.toStdString());
}

BOOST_AUTO_TEST_CASE(test_toolchain_difference_is_reported) {
  const TextValues current = currentToolchain();
  BOOST_REQUIRE(!current.empty());
  BOOST_CHECK(toolchainDifferences(current, current).isEmpty());

  TextValues recorded = current;
  recorded["qt_runtime"] = "1.2.3";
  const QStringList differences = toolchainDifferences(recorded, current);
  BOOST_REQUIRE_EQUAL(differences.size(), 1);
  BOOST_CHECK(differences[0].contains("qt_runtime"));
  BOOST_CHECK(differences[0].contains("1.2.3"));
  BOOST_CHECK(differences[0].contains(current.at("qt_runtime")));

  // source_commit and build_type change legitimately between a recording and a check.
  TextValues other = current;
  other["source_commit"] = "0123456789012345678901234567890123456789";
  other["build_type"] = "SomethingElse";
  BOOST_CHECK(toolchainDifferences(other, current).isEmpty());

  // A key the recording lacks is a difference too.
  TextValues incomplete = current;
  incomplete.erase("boost");
  const QStringList missing = toolchainDifferences(incomplete, current);
  BOOST_REQUIRE_EQUAL(missing.size(), 1);
  BOOST_CHECK(missing[0].contains("boost"));
}

BOOST_AUTO_TEST_CASE(test_normal_run_never_writes) {
  const Roots roots;
  GoldenStore normal(roots.goldenPath(), roots.artifactPath(), false);

  const GoldenStore::Outcome missing =
      checkPage(normal, "suite_a", "page_one", 0);
  BOOST_CHECK(!missing.pass);
  BOOST_CHECK(!missing.written);
  BOOST_CHECK_MESSAGE(missing.message.contains("UPDATE_GOLDENS=1"), missing.message.toStdString());
  BOOST_CHECK(snapshotOf(roots.goldenPath()).empty());
  BOOST_CHECK(QFileInfo::exists(roots.artifactPath() + "/suite_a/page_one/actual.png"));

  recordImage(roots, "suite_a", "page_one", 0);
  const Snapshot before = snapshotOf(roots.goldenPath());
  GoldenStore second(roots.goldenPath(), roots.artifactPath(), false);
  BOOST_CHECK(!checkPage(second, "suite_a", "page_one", 1).pass);
  BOOST_CHECK(!second.checkText("suite_a", "decisions", TextValues{{"angle_deg", "1.0000"}}, sanityOk()).pass);
  BOOST_CHECK(snapshotOf(roots.goldenPath()) == before);
}

BOOST_AUTO_TEST_CASE(test_failure_writes_artifacts_and_reports_metric) {
  const Roots roots;
  recordImage(roots, "suite_a", "page_one", 0);
  {
    GoldenStore update(roots.goldenPath(), roots.artifactPath(), true);
    BOOST_REQUIRE(update.checkText("suite_a", "decisions", TextValues{{"angle_deg", "1.0000"}}, sanityOk()).written);
  }
  const Snapshot before = snapshotOf(roots.goldenPath());

  GoldenStore normal(roots.goldenPath(), roots.artifactPath(), false);
  const GoldenStore::Outcome image =
      checkPage(normal, "suite_a", "page_one", 1);
  BOOST_CHECK(!image.pass);
  BOOST_CHECK(!image.metrics.empty());
  const QString folder = roots.artifactPath() + "/suite_a/page_one";
  const std::string message = image.message.toStdString();
  BOOST_CHECK_MESSAGE(image.message.contains("edge_tolerant_xor_fraction"), message);
  BOOST_CHECK_MESSAGE(image.message.contains("limit"), message);
  BOOST_CHECK_MESSAGE(image.message.contains(folder), message);
  for (const char* file : {"expected.png", "actual.png", "diff.png"}) {
    BOOST_CHECK_MESSAGE(QFileInfo::exists(folder + "/" + file), file << " missing in " << folder.toStdString());
  }

  const GoldenStore::Outcome text =
      normal.checkText("suite_a", "decisions", TextValues{{"angle_deg", "2.0000"}}, sanityOk());
  BOOST_CHECK(!text.pass);
  BOOST_CHECK_MESSAGE(text.message.contains("text:angle_deg"), text.message.toStdString());
  const QString textFolder = roots.artifactPath() + "/suite_a/decisions";
  for (const char* file : {"expected.txt", "actual.txt", "diff.txt"}) {
    BOOST_CHECK_MESSAGE(QFileInfo::exists(textFolder + "/" + file), file << " missing in " << textFolder.toStdString());
  }
  const QString diff = readFile(textFolder + "/diff.txt");
  BOOST_CHECK(diff.contains("- angle_deg = 1.0000"));
  BOOST_CHECK(diff.contains("+ angle_deg = 2.0000"));

  BOOST_CHECK(snapshotOf(roots.goldenPath()) == before);
}

BOOST_AUTO_TEST_CASE(test_update_records_missing_golden_and_manifest) {
  const Roots roots;
  GoldenStore update(roots.goldenPath(), roots.artifactPath(), true);
  const GoldenStore::Outcome image = checkPage(update, "suite_a", "page_one", 0);
  BOOST_CHECK_MESSAGE(image.pass && image.written, image.message.toStdString());
  BOOST_CHECK(QFileInfo::exists(roots.goldenPath() + "/suite_a/page_one.png"));
  const GoldenStore::Outcome text =
      update.checkText("suite_a", "decisions", TextValues{{"output_dpi", "600"}}, sanityOk());
  BOOST_CHECK_MESSAGE(text.pass && text.written, text.message.toStdString());
  BOOST_CHECK(QFileInfo::exists(roots.goldenPath() + "/suite_a/decisions.txt"));

  bool ok = false;
  const TextValues manifest = parseText(readFile(roots.goldenPath() + "/suite_a/MANIFEST.txt").toUtf8(), &ok);
  BOOST_REQUIRE(ok);
  TextValues expected = currentToolchain();
  expected["source_commit"] = currentSourceCommit();
  BOOST_CHECK_MESSAGE(manifest == expected, "manifest differs from the current toolchain plus source_commit");

  GoldenStore normal(roots.goldenPath(), roots.artifactPath(), false);
  const GoldenStore::Outcome again = checkPage(normal, "suite_a", "page_one", 0);
  BOOST_CHECK_MESSAGE(again.pass, again.message.toStdString());
  BOOST_CHECK(!again.written);
  BOOST_CHECK_MESSAGE(again.warnings.isEmpty(), again.warnings.join("; ").toStdString());
}

BOOST_AUTO_TEST_CASE(test_update_rewrites_only_failing) {
  const Roots roots;
  {
    GoldenStore first(roots.goldenPath(), roots.artifactPath(), true);
    BOOST_REQUIRE(checkPage(first, "suite_a", "page_one", 0).written);
    BOOST_REQUIRE(checkPage(first, "suite_a", "page_two", 0).written);
  }
  // Make the manifest stale, so that its refresh is observable.
  const QString manifestPath = roots.goldenPath() + "/suite_a/MANIFEST.txt";
  bool ok = false;
  TextValues stale = parseText(readFile(manifestPath).toUtf8(), &ok);
  BOOST_REQUIRE(ok);
  stale["qt_runtime"] = "0.0.1";
  writeFile(manifestPath, serializeText(stale));
  QThread::msleep(30);  // a rewrite must show in the modification time
  const Snapshot before = snapshotOf(roots.goldenPath());

  GoldenStore second(roots.goldenPath(), roots.artifactPath(), true);
  const GoldenStore::Outcome passing =
      checkPage(second, "suite_a", "page_one", 0);
  BOOST_CHECK_MESSAGE(passing.pass && !passing.written, passing.message.toStdString());
  const GoldenStore::Outcome failing =
      checkPage(second, "suite_a", "page_two", 1);
  BOOST_CHECK_MESSAGE(failing.pass && failing.written, failing.message.toStdString());

  const Snapshot after = snapshotOf(roots.goldenPath());
  BOOST_CHECK_MESSAGE(after.at("suite_a/page_one.png") == before.at("suite_a/page_one.png"),
                      "a golden that passes must stay byte- and time-identical");
  BOOST_CHECK(after.at("suite_a/page_two.png") != before.at("suite_a/page_two.png"));
  BOOST_CHECK_EQUAL(parseText(after.at("suite_a/MANIFEST.txt").content, &ok).at("qt_runtime").toStdString(),
                    currentToolchain().at("qt_runtime").toStdString());

  GoldenStore normal(roots.goldenPath(), roots.artifactPath(), false);
  BOOST_CHECK(checkPage(normal, "suite_a", "page_two", 1).pass);
  BOOST_CHECK(!checkPage(normal, "suite_a", "page_two", 0).pass);
}

BOOST_AUTO_TEST_CASE(test_update_twice_is_idempotent) {
  const Roots roots;
  const auto run = [&roots](bool expectWritten) {
    GoldenStore update(roots.goldenPath(), roots.artifactPath(), true);
    const GoldenStore::Outcome a = checkPage(update, "suite_a", "page_one", 0);
    const GoldenStore::Outcome b = checkPage(update, "suite_a", "page_two", 1);
    const GoldenStore::Outcome c =
        update.checkText("suite_b", "decisions", TextValues{{"angle_deg", "1.5000"}}, sanityOk());
    for (const GoldenStore::Outcome* outcome : {&a, &b, &c}) {
      BOOST_CHECK_MESSAGE(outcome->pass, outcome->message.toStdString());
      BOOST_CHECK_EQUAL(outcome->written, expectWritten);
    }
  };
  run(true);
  QThread::msleep(30);
  const Snapshot first = snapshotOf(roots.goldenPath());
  run(false);
  BOOST_CHECK_MESSAGE(snapshotOf(roots.goldenPath()) == first, "the second update run changed a file");
}

BOOST_AUTO_TEST_CASE(test_sanity_failure_blocks_write) {
  const Roots roots;
  const Sanity blank{false, "page is blank"};
  GoldenStore update(roots.goldenPath(), roots.artifactPath(), true);

  const GoldenStore::Outcome missing = checkPage(update, "suite_a", "page_one", 0, blank);
  BOOST_CHECK(!missing.pass);
  BOOST_CHECK(!missing.written);
  BOOST_CHECK_MESSAGE(missing.message.contains("sanity failed: page is blank"), missing.message.toStdString());
  BOOST_CHECK(snapshotOf(roots.goldenPath()).empty());

  const GoldenStore::Outcome text = update.checkText("suite_a", "decisions", TextValues{{"a", "1"}}, blank);
  BOOST_CHECK(!text.pass && !text.written);
  BOOST_CHECK(snapshotOf(roots.goldenPath()).empty());

  // An existing golden that now fails is not replaced by a result that failed the sanity check either.
  recordImage(roots, "suite_a", "page_one", 0);
  const Snapshot before = snapshotOf(roots.goldenPath());
  GoldenStore second(roots.goldenPath(), roots.artifactPath(), true);
  const GoldenStore::Outcome failing = checkPage(second, "suite_a", "page_one", 1, blank);
  BOOST_CHECK(!failing.pass && !failing.written);
  BOOST_CHECK(snapshotOf(roots.goldenPath()) == before);
}

BOOST_AUTO_TEST_CASE(test_update_leaves_no_temporary_files) {
  const Roots roots;
  GoldenStore update(roots.goldenPath(), roots.artifactPath(), true);
  BOOST_REQUIRE(checkPage(update, "suite_a", "page_one", 0).written);
  BOOST_REQUIRE(update.checkText("suite_a", "decisions", TextValues{{"a", "1"}}, sanityOk()).written);
  QImage color(40, 30, QImage::Format_RGB32);
  color.fill(qRgb(10, 200, 30));
  BOOST_REQUIRE(update.checkImage("suite_a", "page_color", color, ImageKind::COLOR, sanityOk()).written);

  const QStringList files =
      QDir(roots.goldenPath() + "/suite_a").entryList(QDir::Files | QDir::Hidden, QDir::Name);
  BOOST_CHECK_EQUAL(files.join(",").toStdString(), "MANIFEST.txt,decisions.txt,page_color.png,page_one.png");

  GoldenStore normal(roots.goldenPath(), roots.artifactPath(), false);
  BOOST_CHECK(normal.checkImage("suite_a", "page_color", color, ImageKind::COLOR, sanityOk()).pass);
}

BOOST_AUTO_TEST_CASE(test_toolchain_mismatch_warns_and_compares) {
  const Roots roots;
  recordImage(roots, "suite_a", "page_one", 0);
  recordImage(roots, "suite_a", "page_two", 0);
  const QString manifestPath = roots.goldenPath() + "/suite_a/MANIFEST.txt";
  bool ok = false;
  TextValues recorded = parseText(readFile(manifestPath).toUtf8(), &ok);
  BOOST_REQUIRE(ok);
  recorded["qt_runtime"] = "0.0.1";
  writeFile(manifestPath, serializeText(recorded));

  GoldenStore normal(roots.goldenPath(), roots.artifactPath(), false);
  const GoldenStore::Outcome first = checkPage(normal, "suite_a", "page_one", 0);
  BOOST_CHECK_MESSAGE(first.pass, first.message.toStdString());
  BOOST_REQUIRE_EQUAL(first.warnings.size(), 1);
  BOOST_CHECK(first.warnings[0].contains("qt_runtime"));
  BOOST_CHECK(!first.metrics.empty());

  const GoldenStore::Outcome second = checkPage(normal, "suite_a", "page_two", 1);
  BOOST_CHECK(!second.pass);  // the comparison still runs and still fails
  BOOST_CHECK_MESSAGE(second.warnings.isEmpty(), "the warning belongs to the first outcome of the suite only");

  // A suite without any manifest also warns, once.
  GoldenStore other(roots.goldenPath(), roots.artifactPath(), false);
  const GoldenStore::Outcome noManifest = checkPage(other, "suite_b", "page_one", 0);
  BOOST_CHECK_EQUAL(noManifest.warnings.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_invalid_name_rejected) {
  const Roots roots;
  GoldenStore update(roots.goldenPath(), roots.artifactPath(), true);
  const QStringList bad{"../x", "X", "", "a/b", "a b", "name\n", ".", "..", "x.png"};
  for (const QString& component : bad) {
    const GoldenStore::Outcome asName = checkPage(update, "suite_a", component, 0);
    BOOST_CHECK_MESSAGE(!asName.pass && !asName.written, "name accepted: " << component.toStdString());
    const GoldenStore::Outcome asSuite = checkPage(update, component, "page_one", 0);
    BOOST_CHECK_MESSAGE(!asSuite.pass && !asSuite.written, "suite accepted: " << component.toStdString());
    const GoldenStore::Outcome asText = update.checkText(component, component, TextValues{{"a", "1"}}, sanityOk());
    BOOST_CHECK_MESSAGE(!asText.pass && !asText.written, "text names accepted: " << component.toStdString());
  }
  BOOST_CHECK(snapshotOf(roots.goldenPath()).empty());
  BOOST_CHECK(snapshotOf(roots.artifactPath()).empty());
  BOOST_CHECK(!QFileInfo::exists(roots.goldenPath() + "/x.png"));
  BOOST_CHECK(!QFileInfo::exists(roots.goldenPath() + "/../x"));
}

BOOST_AUTO_TEST_CASE(test_update_flag_parsing) {
  const bool wasSet = qEnvironmentVariableIsSet("UPDATE_GOLDENS");
  const QByteArray saved = qgetenv("UPDATE_GOLDENS");

  qunsetenv("UPDATE_GOLDENS");
  BOOST_CHECK(!GoldenStore::updateRequestedFromEnvironment());
  for (const char* value : {"0", "true", "", "1 ", "yes", "11"}) {
    qputenv("UPDATE_GOLDENS", value);
    BOOST_CHECK_MESSAGE(!GoldenStore::updateRequestedFromEnvironment(), "'" << value << "' must not enable updates");
  }
  qputenv("UPDATE_GOLDENS", "1");
  BOOST_CHECK(GoldenStore::updateRequestedFromEnvironment());

  if (wasSet) {
    qputenv("UPDATE_GOLDENS", saved);
  } else {
    qunsetenv("UPDATE_GOLDENS");
  }
}

// XFAIL_CHECK(true, ...) raises exactly one Boost error; expected_failures(1) turns that into the pass of this case.
BOOST_AUTO_TEST_CASE(test_expected_failure_trip_wire, *boost::unit_test::expected_failures(1)) {
  BOOST_CHECK(expectedFailureStatus(false) == ExpectedFailureStatus::KNOWN_DEFECT);
  BOOST_CHECK(expectedFailureStatus(true) == ExpectedFailureStatus::UNEXPECTED_PASS);
  XFAIL_CHECK(false, "no requirement: defect still present (logs only)");
  XFAIL_CHECK(true, "no requirement: raised on purpose by the test of the trip-wire");
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace tests
}  // namespace test_support
