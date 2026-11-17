// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "XmlMarshaller.h"

#include <QLineF>
#include <QPointF>
#include <QPolygonF>
#include <QRect>
#include <QString>

#include "Utils.h"

using namespace foundation;

QDomElement XmlMarshaller::string(const QString& str, const QString& name) {
  QDomElement el(m_doc.createElement(name));
  el.appendChild(m_doc.createTextNode(str));
  return el;
}

// Only the XML-1.0-forbidden characters need this.  QDom already escapes &, <, ", tab, LF and CR in attribute values
// (as character references) and the parser restores them, but it writes the forbidden characters raw, and the parser
// rejects those and their character references alike.
bool XmlMarshaller::needsEscaping(const QString& value) {
  for (const QChar ch : value) {
    const char16_t unit = ch.unicode();
    if ((unit < 0x20 && unit != 0x09 && unit != 0x0a && unit != 0x0d) || unit == 0xfffe || unit == 0xffff) {
      return true;
    }
  }
  return false;
}

QString XmlMarshaller::escapeInvalidXmlChars(const QString& value) {
  return QString::fromLatin1(value.toUtf8().toPercentEncoding());
}

void XmlMarshaller::setEscapedAttribute(QDomElement& el, const QString& name, const QString& value) {
  if (needsEscaping(value)) {
    el.setAttribute(name, escapeInvalidXmlChars(value));
    el.setAttribute(name + QStringLiteral("Escaped"), QStringLiteral("1"));
  } else {
    el.setAttribute(name, value);
  }
}

QDomElement XmlMarshaller::size(const QSize& size, const QString& name) {
  if (size.isNull()) {
    return QDomElement();
  }

  QDomElement el(m_doc.createElement(name));
  el.setAttribute("width", size.width());
  el.setAttribute("height", size.height());
  return el;
}

QDomElement XmlMarshaller::sizeF(const QSizeF& size, const QString& name) {
  if (size.isNull()) {
    return QDomElement();
  }

  QDomElement el(m_doc.createElement(name));
  el.setAttribute("width", Utils::doubleToString(size.width()));
  el.setAttribute("height", Utils::doubleToString(size.height()));
  return el;
}

QDomElement XmlMarshaller::pointF(const QPointF& p, const QString& name) {
  QDomElement el(m_doc.createElement(name));
  el.setAttribute("x", Utils::doubleToString(p.x()));
  el.setAttribute("y", Utils::doubleToString(p.y()));
  return el;
}

QDomElement XmlMarshaller::lineF(const QLineF& line, const QString& name) {
  QDomElement el(m_doc.createElement(name));
  el.appendChild(pointF(line.p1(), "p1"));
  el.appendChild(pointF(line.p2(), "p2"));
  return el;
}

QDomElement XmlMarshaller::rect(const QRect& rect, const QString& name) {
  QDomElement el(m_doc.createElement(name));
  el.setAttribute("x", QString::number(rect.x()));
  el.setAttribute("y", QString::number(rect.y()));
  el.setAttribute("width", QString::number(rect.width()));
  el.setAttribute("height", QString::number(rect.height()));
  return el;
}

QDomElement XmlMarshaller::rectF(const QRectF& rect, const QString& name) {
  QDomElement el(m_doc.createElement(name));
  el.setAttribute("x", Utils::doubleToString(rect.x()));
  el.setAttribute("y", Utils::doubleToString(rect.y()));
  el.setAttribute("width", Utils::doubleToString(rect.width()));
  el.setAttribute("height", Utils::doubleToString(rect.height()));
  return el;
}

QDomElement XmlMarshaller::polygonF(const QPolygonF& poly, const QString& name) {
  QDomElement el(m_doc.createElement(name));

  QPolygonF::const_iterator it(poly.begin());
  const QPolygonF::const_iterator end(poly.end());
  for (; it != end; ++it) {
    el.appendChild(pointF(*it, "point"));
  }
  return el;
}
