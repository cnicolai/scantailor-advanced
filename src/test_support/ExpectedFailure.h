// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_EXPECTEDFAILURE_H_
#define SCANTAILOR_TEST_SUPPORT_EXPECTEDFAILURE_H_

// Interface stub for the RED commit of plan 02-02 task 3.

namespace test_support {

enum class ExpectedFailureStatus { KNOWN_DEFECT, UNEXPECTED_PASS };

inline ExpectedFailureStatus expectedFailureStatus(bool) {
  return ExpectedFailureStatus::KNOWN_DEFECT;
}

}  // namespace test_support

#define XFAIL_CHECK(cond, tag) static_cast<void>(0)

#endif  // SCANTAILOR_TEST_SUPPORT_EXPECTEDFAILURE_H_
