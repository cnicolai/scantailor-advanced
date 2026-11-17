// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "XmlUnmarshaller.h"

#include <QByteArray>
#include <QDomElement>
#include <QLineF>
#include <QPointF>
#include <QPolygonF>
#include <QRect>
#include <QString>

QString XmlUnmarshaller::string(const QDomElement& el) {
  // QDomElement::text() already returns unescaped text: the parser resolves character and entity references.
  return el.text();
}

QString XmlUnmarshaller::unescapeInvalidXmlChars(const QString& text) {
  return QString::fromUtf8(QByteArray::fromPercentEncoding(text.toUtf8()));
}

QString XmlUnmarshaller::escapedAttribute(const QDomElement& el, const QString& name) {
  const QString text = el.attribute(name);
  if (el.attribute(name + QStringLiteral("Escaped")) == QLatin1String("1")) {
    return unescapeInvalidXmlChars(text);
  }
  return text;
}

QSize XmlUnmarshaller::size(const QDomElement& el) {
  const int width = el.attribute("width").toInt();
  const int height = el.attribute("height").toInt();
  return QSize(width, height);
}

QSizeF XmlUnmarshaller::sizeF(const QDomElement& el) {
  const double width = el.attribute("width").toDouble();
  const double height = el.attribute("height").toDouble();
  return QSizeF(width, height);
}

QPointF XmlUnmarshaller::pointF(const QDomElement& el) {
  const double x = el.attribute("x").toDouble();
  const double y = el.attribute("y").toDouble();
  return QPointF(x, y);
}

QLineF XmlUnmarshaller::lineF(const QDomElement& el) {
  const QPointF p1(pointF(el.namedItem("p1").toElement()));
  const QPointF p2(pointF(el.namedItem("p2").toElement()));
  return QLineF(p1, p2);
}

QRect XmlUnmarshaller::rect(const QDomElement& el) {
  const int x = el.attribute("x").toInt();
  const int y = el.attribute("y").toInt();
  const int width = el.attribute("width").toInt();
  const int height = el.attribute("height").toInt();
  return QRect(x, y, width, height);
}

QRectF XmlUnmarshaller::rectF(const QDomElement& el) {
  const double x = el.attribute("x").toDouble();
  const double y = el.attribute("y").toDouble();
  const double width = el.attribute("width").toDouble();
  const double height = el.attribute("height").toDouble();
  return QRectF(x, y, width, height);
}

QPolygonF XmlUnmarshaller::polygonF(const QDomElement& el) {
  QPolygonF poly;

  const QString pointTagName("point");
  QDomNode node(el.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != pointTagName) {
      continue;
    }
    poly.push_back(pointF(node.toElement()));
  }
  return poly;
}
