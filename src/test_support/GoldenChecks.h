// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_GOLDENCHECKS_H_
#define SCANTAILOR_TEST_SUPPORT_GOLDENCHECKS_H_

#include <QString>
#include <boost/test/unit_test.hpp>
#include <string>

#include "GoldenStore.h"

// Boost.Test reporting for GoldenStore outcomes.  Header-only and for test sources only: the .cpp files of
// test_support never include Boost.Test.
//
// Log lines (message level), one per metric of every comparison, passing or failing, parsed by plan 02-09:
//   GOLDEN-METRIC suite=<s> name=<n> kind=<bilevel|color|text> metric=<m> value=<v> limit=<l> result=<pass|fail>
// and, when they apply:
//   TOOLCHAIN DIFFERS suite=<s>: <entry>
//   RECORDED <suite>/<name>

namespace test_support {

namespace detail {

inline std::string utf8(const QString& text) {
  return text.toUtf8().toStdString();
}

inline std::string number(double value) {
  return utf8(QString::number(value, 'g', 10));
}

}  // namespace detail

inline const char* kindLabel(ImageKind kind) {
  return kind == ImageKind::BILEVEL ? "bilevel" : "color";
}

#if defined(GOLDEN_DIR) && defined(ARTIFACT_DIR)
/// The store of the real golden directory.  Update mode comes from UPDATE_GOLDENS=1 and is read once.
inline GoldenStore& pipelineGoldenStore() {
  static GoldenStore store(QString::fromUtf8(GOLDEN_DIR),
                           QString::fromUtf8(ARTIFACT_DIR),
                           GoldenStore::updateRequestedFromEnvironment());
  return store;
}
#endif

inline void reportOutcome(const QString& suite,
                          const QString& name,
                          const char* kind,
                          const GoldenStore::Outcome& outcome) {
  for (const MetricResult& metric : outcome.metrics) {
    BOOST_TEST_MESSAGE("GOLDEN-METRIC suite=" << detail::utf8(suite) << " name=" << detail::utf8(name)
                                              << " kind=" << kind << " metric=" << detail::utf8(metric.metric)
                                              << " value=" << detail::number(metric.value)
                                              << " limit=" << detail::number(metric.limit)
                                              << " result=" << (metric.pass ? "pass" : "fail"));
  }
  for (const QString& warning : outcome.warnings) {
    BOOST_TEST_MESSAGE("TOOLCHAIN DIFFERS suite=" << detail::utf8(suite) << ": " << detail::utf8(warning));
  }
  if (outcome.written) {
    BOOST_TEST_MESSAGE("RECORDED " << detail::utf8(suite) << "/" << detail::utf8(name));
  }
  BOOST_CHECK_MESSAGE(outcome.pass, detail::utf8(suite) << "/" << detail::utf8(name) << ": "
                                                        << detail::utf8(outcome.message));
}

}  // namespace test_support

#define CHECK_IMAGE_GOLDEN(suite, name, image, kind, sanity)                                                     \
  do {                                                                                                           \
    const QString goldenSuite_(suite);                                                                           \
    const QString goldenName_(name);                                                                             \
    ::test_support::reportOutcome(                                                                               \
        goldenSuite_, goldenName_, ::test_support::kindLabel(kind),                                              \
        ::test_support::pipelineGoldenStore().checkImage(goldenSuite_, goldenName_, (image), (kind), (sanity))); \
  } while (0)

#define CHECK_TEXT_GOLDEN(suite, name, values, sanity)                                                   \
  do {                                                                                                   \
    const QString goldenSuite_(suite);                                                                   \
    const QString goldenName_(name);                                                                     \
    ::test_support::reportOutcome(                                                                       \
        goldenSuite_, goldenName_, "text",                                                               \
        ::test_support::pipelineGoldenStore().checkText(goldenSuite_, goldenName_, (values), (sanity))); \
  } while (0)

#endif  // SCANTAILOR_TEST_SUPPORT_GOLDENCHECKS_H_
