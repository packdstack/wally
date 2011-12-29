/*
 * Wally - Qt4 wallpaper/background changer
 * Copyright (C) 2009  Antonio Di Monaco <tony@becrux.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QVectorIterator>
#include <QMapIterator>

#include "json.h"

using namespace JSON;

char Element::readNextChar(QIODevice *device)
{
  char c;

  if (!device->getChar(&c))
    throw QString("Element::readString: unexpected EOF at pos %1").arg(device->pos());

  return c;
}

char Element::readNext(QIODevice *device)
{
  char c;

  do

    device->getChar(&c);

  while (!device->atEnd() && QChar(c).isSpace());

  return c;
}

QString Element::readString(QIODevice *device)
{
  QString res;
  QByteArray resB;
  char c;
  bool falseQuote;

  if ((c = readNextChar(device)) != '"')
    throw QString("Element::readString: unexpected char '%1' at pos %2").arg(c).arg(device->pos());

  do
  {
    falseQuote = false;
    c = readNextChar(device);

    if (c == '\\')
      switch (readNextChar(device))
      {
        case 't':
          resB += c = 9;
          break;

        case '"':
          falseQuote = true;
          resB += c = '"';
          break;

        case '/':
          resB += c = '/';
          break;

        case 'b':
          resB += c = 8;
          break;

        case 'f':
          resB += c = 12;
          break;

        case 'n':
          resB += c = 10;
          break;

        case 'r':
          resB += c = 13;
          break;

        case '\\':
          resB += c = '\\';
          break;

        case 'u':
        {
          res += QString::fromUtf8(resB);
          resB.clear();

          QByteArray uCode;

          device->getChar(&c);
          uCode += c;
          device->getChar(&c);
          uCode += c;
          QString hd1 = QString::fromUtf8(uCode);

          uCode.clear();
          device->getChar(&c);
          uCode += c;
          device->getChar(&c);
          uCode += c;
          QString hd2 = QString::fromUtf8(uCode);

          ushort uCode1 = hd1.toUShort(0,16);
          ushort uCode2 = hd2.toUShort(0,16);

          res += QChar(uCode2,uCode1);
        }
        break;
      }

    else
      resB += c;
  }
  while (!device->atEnd() && ((c != '"') || ((c == '"') && falseQuote)));

  res += QString::fromUtf8(resB);
  res.chop(1);

  return res;
}

QVariant Element::readNumber(QIODevice *device)
{
  QVariant res;
  QString str;
  char c;
  bool iOk, rOk;
  int iN;
  double rN;

  do

    str += c = readNextChar(device);

  while (!device->atEnd() && ((c != ' ') && (c != ',') && (c != '}') && (c != ']')));
  str.chop(1);
  device->ungetChar(c);

  iN = str.toInt(&iOk);
  rN = str.toDouble(&rOk);

  if ((!iOk) && (!rOk))
    throw QString("Element::readNumber: unexpected value at pos %1").arg(device->pos());

  if (iOk)
    res.setValue(iN);
  else
    res.setValue(rN);

  return res;
}

void Value::read(QIODevice *device)
{
  QVariant var;
  char c;

  switch (c = readNext(device))
  {
    case '"':
      device->ungetChar(c);
      setValue(readString(device));
      break;

    case 't':
      if (((c = readNextChar(device)) != 'r') ||
          ((c = readNextChar(device)) != 'u') ||
          ((c = readNextChar(device)) != 'e'))
        throw QString("Value::read: unexpected char '%1' at pos %2").arg(c).arg(device->pos());

      setValue(true);
      break;

    case 'f':
      if (((c = readNextChar(device)) != 'a') ||
          ((c = readNextChar(device)) != 'l') ||
          ((c = readNextChar(device)) != 's') ||
          ((c = readNextChar(device)) != 'e'))
        throw QString("Value::read: unexpected char '%1' at pos %2").arg(c).arg(device->pos());

      setValue(false);
      break;

    case 'n':
      if (((c = readNextChar(device)) != 'u') ||
          ((c = readNextChar(device)) != 'l') ||
          ((c = readNextChar(device)) != 'l'))
        throw QString("Value::read: unexpected char '%1' at pos %2").arg(c).arg(device->pos());
      break;

    default:
      device->ungetChar(c);
      var = readNumber(device);

      switch (var.type())
      {
        case QVariant::Double:
          setValue(var.value<double>());
          break;

        case QVariant::Int:
          setValue(var.value<int>());
          break;

        default:
          break;
      }
      break;
  }
}

void Value::write(QIODevice *device)
{
  switch (type())
  {
    case QVariant::Bool:
      device->write(QByteArray((toBool())? "true" : "false"));
      break;

    case QVariant::Double:
      device->write(QString::number(toDouble()).toAscii());
      break;

    case QVariant::Int:
      device->write(QString::number(toInt()).toAscii());
      break;

    case QVariant::String:
      device->write("\"");
      device->write(toString().toAscii());
      device->write("\"");
      break;

    default:
      break;
  }
}

void Array::read(QIODevice *device)
{
  char c;
  Element *element;

  c = readNext(device);

  if (c != '[')
    throw QString("Array::read: unexpected char '%1' at pos %2").arg(c).arg(device->pos());

  do
  {
    c = readNext(device);
    if (c == ']')
      break;

    device->ungetChar(c);

    if (c == '{')
      element = new Object;
    else if (c == '[')
      element = new Array;
    else
      element = new Value;
    element->read(device);

    append(element);

    if ((c != ',') && (c != ']'))
      c = readNext(device);
  }
  while (!device->atEnd() && ((c == ',') || (c != ']')));

  if (c != ']')
    throw QString("Array::read: unexpected char '%1' at pos %2").arg(c).arg(device->pos());
}

void Array::write(QIODevice *device)
{
  QVectorIterator< JSON::Element * > item(*this);

  device->write("[");

  while (item.hasNext())
  {
    item.next()->write(device);

    if (item.hasNext())
      device->write(",");
  }

  device->write("]");
}

void Object::read(QIODevice *device)
{
  char c;
  QString key;
  Element *element;

  c = readNext(device);

  if (c != '{')
    throw QString("Object::read: unexpected char '%1' at pos %2").arg(c).arg(device->pos());

  do
  {
    c = readNext(device);
    if (c == '}')
      break;

    device->ungetChar(c);

    key = readString(device);

    c = readNext(device);

    if (c != ':')
      throw QString("Object::read: unexpected char '%1' at pos %2").arg(c).arg(device->pos());

    c = readNext(device);
    device->ungetChar(c);

    if (c == '{')
      element = new Object;
    else if (c == '[')
      element = new Array;
    else
      element = new Value;
    element->read(device);

    insert(key,element);

    c = readNext(device);
  }
  while (!device->atEnd() && ((c != '}') || (c == ',')));

  if (c != '}')
    throw QString("Object::read: unexpected char '%1' at pos %2").arg(c).arg(device->pos());
}

void Object::write(QIODevice *device)
{
  QMapIterator< QString,JSON::Element * > item(*this);

  device->write("{");

  while (item.hasNext())
  {
    item.next();
    device->write("\"");
    device->write(item.key().toAscii());
    device->write("\":");
    item.value()->write(device);

    if (item.hasNext())
      device->write(",");
  }

  device->write("}");
}

void Reader::parse()
{
  char c;

  c = Element::readNext(_device);
  if (!_device->atEnd())
    _device->ungetChar(c);

  if (!_device->atEnd())
  {
    switch (c)
    {
      case '{':
        _element = new Object;
        break;

      case '[':
        _element = new Array;
        break;

      default:
        throw QString("Reader::parse: unexpected char '%1' at pos %2").arg(c).arg(_device->pos());
        break;
    }

    _element->read(_device);
  }
}

void Writer::write(Element *element)
{
  element->write(_device);
}
