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

#ifndef JSON_H
#define JSON_H

#include <QString>
#include <QVariant>
#include <QTextStream>
#include <QVector>
#include <QMap>

namespace JSON
{
  class Value;
  class Array;
  class Object;
  class Reader;
  class Writer;

  class Element
  {
    friend class Value;
    friend class Array;
    friend class Object;
    friend class Reader;
    friend class Writer;

  protected:
    virtual void read(QIODevice *device) = 0;
    virtual void write(QIODevice *device) = 0;

    static char readNextChar(QIODevice *device);
    static char readNext(QIODevice *device);
    static QString readString(QIODevice *device);
    static QVariant readNumber(QIODevice *device);

  public:
    enum ElementType { ElementValue, ElementArray, ElementObject };

    virtual ~Element() { }

    virtual ElementType elementType() const = 0;
  };

  class Value : public QVariant, public Element
  {
  protected:
    void read(QIODevice *device);
    void write(QIODevice *device);

  public:
    ElementType elementType() const { return ElementValue; }
  };

  class Array : public QVector< Element * >, public Element
  {
  protected:
    void read(QIODevice *device);
    void write(QIODevice *device);

  public:
    virtual ~Array() { qDeleteAll(*this); }

    ElementType elementType() const { return ElementArray; }
  };

  class Object : public QMap< QString, Element * >, public Element
  {
  protected:
    void read(QIODevice *device);
    void write(QIODevice *device);

  public:
    virtual ~Object() { qDeleteAll(values()); }

    ElementType elementType() const { return ElementObject; }
  };

  class Reader
  {
    QIODevice *_device;
    Element *_element;

    void parse();

  public:
    Reader(QIODevice *device) : _device(device), _element(0) { parse(); }
    virtual ~Reader() { delete _element; }

    QIODevice *device() const { return _device; }

    Element *element() const { return _element; }
  };

  class Writer
  {
    QIODevice *_device;
    void write(Element *element);

  public:
    Writer(Element *element, QIODevice *device) : _device(device) { write(element); }

    QIODevice *device() const { return _device; }
  };
}

#endif
