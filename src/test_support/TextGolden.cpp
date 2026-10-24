// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "TextGolden.h"

#include <QList>
#include <cmath>

#include "Tolerances.h"

namespace test_support {

namespace {

const char kSeparator[] = " = ";
const int kSeparatorLength = 3;

/// Tolerance for the numeric key suffixes of D-14; false for keys that must match exactly.
bool numericTolerance(const QString& key, double* tolerance) {
  if (key.endsWith("_deg")) {
    *tolerance = tolerance::kGeometryAngleDeg;
  } else if (key.endsWith("_px")) {
    *tolerance = tolerance::kGeometryPx;
  } else if (key.endsWith("_mm")) {
    *tolerance = tolerance::kGeometryMm;
  } else {
    return false;
  }
  return true;
}

MetricResult exactMetric(const QString& name, bool equal) {
  return MetricResult{name, equal ? 0.0 : 1.0, 0.0, equal};
}

}  // namespace

QString formatNumber(double value, int decimals) {
  // QString::number always uses the C locale, so the process locale cannot change the decimal separator.
  QString text = QString::number(value, 'f', decimals);
  bool allZero = true;
  for (const QChar c : text) {
    allZero = allZero && (c == '-' || c == '.' || c == '0');
  }
  if (text.startsWith('-') && allZero) {
    text.remove(0, 1);  // "-0.0000" would be a spurious difference against "0.0000"
  }
  return text;
}

QByteArray serializeText(const TextValues& values) {
  QByteArray out;
  for (const auto& entry : values) {
    out += entry.first.toUtf8();
    out += kSeparator;
    out += entry.second.toUtf8();
    out += '\n';
  }
  return out;
}

TextValues parseText(const QByteArray& data, bool* ok) {
  TextValues values;
  bool good = true;
  const QList<QByteArray> lines = data.split('\n');
  for (const QByteArray& line : lines) {
    if (line.isEmpty()) {
      continue;  // the trailing line break
    }
    const int separator = line.indexOf(kSeparator);
    if (separator <= 0) {  // no separator, or an empty key
      good = false;
      break;
    }
    const QString key = QString::fromUtf8(line.left(separator));
    if (values.count(key)) {
      good = false;
      break;
    }
    values[key] = QString::fromUtf8(line.mid(separator + kSeparatorLength));
  }
  if (!good) {
    values.clear();
  }
  if (ok) {
    *ok = good;
  }
  return values;
}

bool textRoundTrips(const TextValues& values, QString* offendingKey) {
  for (const auto& entry : values) {
    const TextValues single{entry};
    bool ok = false;
    const TextValues parsed = parseText(serializeText(single), &ok);
    if (!ok || parsed != single) {
      if (offendingKey) {
        *offendingKey = entry.first;
      }
      return false;
    }
  }
  return true;
}

CompareResult compareText(const TextValues& expected, const TextValues& actual) {
  CompareResult result{true, {}, QImage()};

  for (const auto& entry : expected) {
    const QString& key = entry.first;
    const auto other = actual.find(key);
    if (other == actual.end()) {
      result.metrics.push_back(exactMetric("key:" + key, false));
      continue;
    }

    double limit = 0.0;
    if (!numericTolerance(key, &limit)) {
      result.metrics.push_back(exactMetric("text:" + key, entry.second == other->second));
      continue;
    }

    bool okExpected = false;
    bool okActual = false;
    const double a = entry.second.toDouble(&okExpected);
    const double b = other->second.toDouble(&okActual);
    if (!okExpected || !okActual) {
      result.metrics.push_back(exactMetric("text:" + key, false));
      continue;
    }
    const double delta = std::abs(a - b);
    const bool within = delta <= limit + tolerance::kTextToleranceSlack;
    result.metrics.push_back(MetricResult{"text:" + key, delta, limit, within});
  }

  for (const auto& entry : actual) {
    if (expected.find(entry.first) == expected.end()) {
      result.metrics.push_back(exactMetric("key:" + entry.first, false));
    }
  }

  for (const MetricResult& metric : result.metrics) {
    result.pass = result.pass && metric.pass;
  }
  return result;
}

QByteArray textDiff(const TextValues& expected, const TextValues& actual) {
  QByteArray out;
  for (const auto& entry : expected) {
    const auto other = actual.find(entry.first);
    if (other == actual.end() || other->second != entry.second) {
      out += "- " + entry.first.toUtf8() + kSeparator + entry.second.toUtf8() + '\n';
    }
  }
  for (const auto& entry : actual) {
    const auto other = expected.find(entry.first);
    if (other == expected.end() || other->second != entry.second) {
      out += "+ " + entry.first.toUtf8() + kSeparator + entry.second.toUtf8() + '\n';
    }
  }
  return out;
}

}  // namespace test_support
