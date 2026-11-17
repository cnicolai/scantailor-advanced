// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <XmlMarshaller.h>
#include <XmlUnmarshaller.h>

#include <QByteArray>
#include <QChar>
#include <QDomDocument>
#include <QDomElement>
#include <QString>
#include <QStringList>
#include <boost/test/unit_test.hpp>
#include <vector>

namespace {

// Every code unit XML 1.0 forbids in a document.
std::vector<char16_t> forbiddenUnits() {
  std::vector<char16_t> units;
  for (char16_t unit = 0x01; unit <= 0x08; ++unit) {
    units.push_back(unit);
  }
  units.push_back(0x0b);
  units.push_back(0x0c);
  for (char16_t unit = 0x0e; unit <= 0x1f; ++unit) {
    units.push_back(unit);
  }
  units.push_back(0xfffe);
  units.push_back(0xffff);
  return units;
}

QString emoji() {
  return QString::fromUtf8("\xF0\x9F\x93\x84");  // U+1F4C4, a surrogate pair in UTF-16
}

QString withUnit(const char16_t unit) {
  return QStringLiteral("a") + QChar(unit) + QStringLiteral("b");
}

bool isPrintableAscii(const QString& text) {
  for (const QChar ch : text) {
    if (ch.unicode() < 0x20 || ch.unicode() > 0x7e) {
      return false;
    }
  }
  return true;
}

// Writes the element to text and parses it again, like a project file on disk.
QDomElement throughXml(QDomDocument& parsed, const QDomDocument& written) {
  const QByteArray bytes = written.toString(2).toUtf8();
  const auto result = parsed.setContent(bytes);
  BOOST_REQUIRE_MESSAGE(static_cast<bool>(result), "not well-formed XML: " << result.errorMessage.toStdString());
  return parsed.documentElement();
}

}  // namespace

BOOST_AUTO_TEST_SUITE(FoundationXmlEscapeTestSuite)

BOOST_AUTO_TEST_CASE(test_ordinary_values_need_no_escaping) {
  const QString nfd = QStringLiteral("a") + QChar(0x0308);
  const QStringList values{QString(),
                           QStringLiteral("a&b <c> \"d\" 'e'"),
                           QStringLiteral("\t"),
                           QStringLiteral("\n"),
                           QStringLiteral("\r"),
                           QStringLiteral("100%"),
                           QString(QChar(0x00e4)),
                           nfd,
                           emoji(),
                           QStringLiteral("  leading and trailing  "),
                           QString(QChar(0x007f))};
  for (const QString& value : values) {
    BOOST_CHECK_MESSAGE(!XmlMarshaller::needsEscaping(value), "needs escaping: " << value.toStdString());
  }
}

BOOST_AUTO_TEST_CASE(test_forbidden_characters_need_escaping) {
  for (const char16_t unit : forbiddenUnits()) {
    BOOST_CHECK_MESSAGE(XmlMarshaller::needsEscaping(withUnit(unit)), "U+" << std::hex << unit << " is not escaped");
  }
}

BOOST_AUTO_TEST_CASE(test_escape_round_trips) {
  for (const char16_t unit : forbiddenUnits()) {
    const QString value = QStringLiteral("x%") + QChar(unit) + QChar(0x00e4) + emoji();
    const QString escaped = XmlMarshaller::escapeInvalidXmlChars(value);
    BOOST_CHECK_MESSAGE(isPrintableAscii(escaped),
                        "escaped text of U+" << std::hex << unit << " is not printable ASCII");
    BOOST_CHECK_MESSAGE(XmlUnmarshaller::unescapeInvalidXmlChars(escaped) == value,
                        "U+" << std::hex << unit << " does not come back");
  }
}

BOOST_AUTO_TEST_CASE(test_attribute_round_trip_through_xml) {
  const QString value = QStringLiteral("in") + QChar(0x0001) + QStringLiteral("dir/") + QChar(0x001f) + emoji();

  QDomDocument doc;
  QDomElement root = doc.createElement("project");
  doc.appendChild(root);
  XmlMarshaller::setEscapedAttribute(root, "path", value);

  QDomDocument parsed;
  const QDomElement element = throughXml(parsed, doc);
  BOOST_CHECK(XmlUnmarshaller::escapedAttribute(element, "path") == value);
  BOOST_CHECK_EQUAL(element.attribute("pathEscaped").toStdString(), "1");
}

BOOST_AUTO_TEST_CASE(test_ordinary_attribute_writes_no_flag) {
  const QString value = QStringLiteral("a&b <c> \"d\"\t\n") + emoji();

  QDomDocument doc;
  QDomElement root = doc.createElement("file");
  doc.appendChild(root);
  XmlMarshaller::setEscapedAttribute(root, "name", value);
  BOOST_CHECK(!root.hasAttribute("nameEscaped"));

  QDomDocument parsed;
  const QDomElement element = throughXml(parsed, doc);
  BOOST_CHECK(!element.hasAttribute("nameEscaped"));
  BOOST_CHECK(XmlUnmarshaller::escapedAttribute(element, "name") == value);
}

BOOST_AUTO_TEST_CASE(test_empty_value) {
  QDomDocument doc;
  QDomElement root = doc.createElement("file");
  doc.appendChild(root);
  XmlMarshaller::setEscapedAttribute(root, "name", QString());
  BOOST_CHECK(root.hasAttribute("name"));
  BOOST_CHECK(!root.hasAttribute("nameEscaped"));

  QDomDocument parsed;
  const QDomElement element = throughXml(parsed, doc);
  BOOST_CHECK(element.hasAttribute("name"));
  BOOST_CHECK(XmlUnmarshaller::escapedAttribute(element, "name").isEmpty());
}

BOOST_AUTO_TEST_CASE(test_unflagged_percent_is_literal) {
  QDomDocument doc;
  QDomElement root = doc.createElement("file");
  doc.appendChild(root);
  XmlMarshaller::setEscapedAttribute(root, "name", QStringLiteral("%01"));
  BOOST_CHECK(!root.hasAttribute("nameEscaped"));

  QDomDocument parsed;
  const QDomElement element = throughXml(parsed, doc);
  BOOST_CHECK_EQUAL(XmlUnmarshaller::escapedAttribute(element, "name").toStdString(), "%01");

  // A flag with any value other than exactly "1" does not decode either.
  QDomElement other = doc.createElement("file");
  other.setAttribute("name", "%01");
  other.setAttribute("nameEscaped", "true");
  BOOST_CHECK_EQUAL(XmlUnmarshaller::escapedAttribute(other, "name").toStdString(), "%01");
}

BOOST_AUTO_TEST_SUITE_END()
