// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_EXPECTEDFAILURE_H_
#define SCANTAILOR_TEST_SUPPORT_EXPECTEDFAILURE_H_

#include <boost/test/unit_test.hpp>

// Trip-wire for a defect of the unmodified code that a later phase will fix.
//
//   XFAIL_CHECK(condition, "tag");
//
// `condition` is the behaviour that should hold once the defect is gone.  While it does not hold, the check logs
// "KNOWN DEFECT <tag>" and passes.  As soon as it holds, the check raises a Boost error, so the mark cannot outlive
// the fix.  The tag names the defect and the requirement that will fix it, for example "special characters in project
// paths are lost (SAFE-02)", or says "no requirement" when none is planned.
//
// Header-only and for test sources only: the .cpp files of test_support never include Boost.Test.

namespace test_support {

enum class ExpectedFailureStatus { KNOWN_DEFECT, UNEXPECTED_PASS };

inline ExpectedFailureStatus expectedFailureStatus(bool conditionHolds) {
  return conditionHolds ? ExpectedFailureStatus::UNEXPECTED_PASS : ExpectedFailureStatus::KNOWN_DEFECT;
}

}  // namespace test_support

#define XFAIL_CHECK(cond, tag)                                              \
  do {                                                                      \
    if (::test_support::expectedFailureStatus(static_cast<bool>(cond))      \
        == ::test_support::ExpectedFailureStatus::KNOWN_DEFECT) {           \
      BOOST_TEST_MESSAGE("KNOWN DEFECT " << tag);                           \
    } else {                                                                \
      BOOST_ERROR("expected failure now passes, remove the mark: " << tag); \
    }                                                                       \
  } while (0)

#endif  // SCANTAILOR_TEST_SUPPORT_EXPECTEDFAILURE_H_
