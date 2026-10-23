// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Manifest.h>
#include <TextGolden.h>
#include <Tolerances.h>

#include <QByteArray>
#include <QLocale>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <boost/test/unit_test.hpp>
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

BOOST_AUTO_TEST_SUITE_END()

}  // namespace tests
}  // namespace test_support
