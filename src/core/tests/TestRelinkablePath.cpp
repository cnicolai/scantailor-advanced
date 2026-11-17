// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <RelinkablePath.h>

#include <QString>
#include <boost/test/unit_test.hpp>

// The path visualisation has no branch for a leading double slash because
// normalisation never produces one. A change to the backslash handling of
// RelinkablePath::normalize must keep these results or bring that branch back.
BOOST_AUTO_TEST_SUITE(CoreRelinkablePathTestSuite)

BOOST_AUTO_TEST_CASE(test_leading_double_slash_collapses) {
  const RelinkablePath path(QStringLiteral("//host/share/x.png"), RelinkablePath::File);
  BOOST_CHECK_EQUAL(path.normalizedPath().toStdString(), "/host/share/x.png");
}

BOOST_AUTO_TEST_CASE(test_backslash_host_path_collapses) {
  const RelinkablePath path(QStringLiteral("\\\\host\\share\\x.png"), RelinkablePath::File);
  BOOST_CHECK_EQUAL(path.normalizedPath().toStdString(), "/host/share/x.png");
}

BOOST_AUTO_TEST_SUITE_END()
