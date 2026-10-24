// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_TEXTGOLDEN_H_
#define SCANTAILOR_TEST_SUPPORT_TEXTGOLDEN_H_

#include <QByteArray>
#include <QString>
#include <map>

#include "ImageCompare.h"

namespace test_support {

/// Stage decisions as readable key/value pairs.  Keys must not be empty or contain " = " or line breaks, values no line
/// breaks.  GoldenStore::checkText refuses values that break this (textRoundTrips).
using TextValues = std::map<QString, QString>;

/// QString::number(value, 'f', decimals), independent of the process locale; a negative zero prints without sign.
QString formatNumber(double value, int decimals = 4);

/// Sorted "key = value\n" lines, UTF-8.
QByteArray serializeText(const TextValues& values);

/// Inverse of serializeText.  *ok (when given) is false for a line without " = ", an empty key or a repeated key; the
/// result is then empty.
TextValues parseText(const QByteArray& data, bool* ok);

/// True when every entry survives serializeText and parseText on its own.  Otherwise false, and *offendingKey (when
/// given) is the first such key in key order.
bool textRoundTrips(const TextValues& values, QString* offendingKey);

/**
 * Per-key tolerance by key suffix: _deg, _px and _mm are numbers compared with the D-14 limits from Tolerances.h
 * (a non-numeric value fails), every other key must match exactly.  Metric names are "text:<key>"; a key present on
 * one side only fails with the metric "key:<name>".
 */
CompareResult compareText(const TextValues& expected, const TextValues& actual);

/// Lines "- key = expected" / "+ key = actual" for every key that differs, for the failure artifact diff.txt.
QByteArray textDiff(const TextValues& expected, const TextValues& actual);

}  // namespace test_support

#endif  // SCANTAILOR_TEST_SUPPORT_TEXTGOLDEN_H_
