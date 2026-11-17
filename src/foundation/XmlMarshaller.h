// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_XMLMARSHALLER_H_
#define SCANTAILOR_FOUNDATION_XMLMARSHALLER_H_

#include <QDomDocument>
#include <QDomElement>
#include <QString>

class QSize;
class QSizeF;
class QPointF;
class QLineF;
class QPolygonF;
class QRect;
class QRectF;

class XmlMarshaller {
 public:
  explicit XmlMarshaller(const QDomDocument& doc) : m_doc(doc) {}

  QDomElement string(const QString& str, const QString& name);

  QDomElement size(const QSize& size, const QString& name);

  QDomElement sizeF(const QSizeF& size, const QString& name);

  QDomElement pointF(const QPointF& p, const QString& name);

  QDomElement lineF(const QLineF& line, const QString& name);

  QDomElement rect(const QRect& rect, const QString& name);

  QDomElement rectF(const QRectF& rect, const QString& name);

  QDomElement polygonF(const QPolygonF& poly, const QString& name);

  /**
   * True if the value contains a character that XML 1.0 forbids: a code unit below U+0020 other than tab, LF and CR,
   * or U+FFFE or U+FFFF.  QDom writes such characters raw and the parser then rejects the file.
   */
  static bool needsEscaping(const QString& value);

  /** Percent-encodes the UTF-8 bytes of the value.  The result is printable ASCII. */
  static QString escapeInvalidXmlChars(const QString& value);

  /**
   * Sets an attribute.  A value that needs escaping is written percent-encoded and the attribute "<name>Escaped" is set
   * to "1", so the reader knows to decode it.  Any other value is written as it is, with no flag, so the files of
   * ordinary projects do not change.
   */
  static void setEscapedAttribute(QDomElement& el, const QString& name, const QString& value);

 private:
  QDomDocument m_doc;
};


#endif  // ifndef SCANTAILOR_FOUNDATION_XMLMARSHALLER_H_
