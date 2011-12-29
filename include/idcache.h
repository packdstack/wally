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

#ifndef IDCACHE_H
#define IDCACHE_H

#include <QMap>
#include <QMapIterator>
#include <QMutableMapIterator>
#include <QDataStream>

template < typename T > class IdCache;
template < typename T > QDataStream &operator<<(QDataStream &out, const IdCache< T > &cache);
template < typename T > QDataStream &operator>>(QDataStream &in, IdCache< T > &cache);

template < typename T > class IdCache
{
  int _length;
  QMap< T,int > _map;

  void cleanOldest();

  void updateCounters(const T &id);

  T findLowestId() const;

public:
  IdCache(int length = 50) : _length(length) { }

  bool contains(const T &id) const;

  void insert(const T &id);

  friend QDataStream &operator<< < T >(QDataStream &out, const IdCache< T > &cache);
  friend QDataStream &operator>> < T >(QDataStream &in, IdCache< T > &cache);
};

template < typename T > T IdCache< T >::findLowestId() const
{
  T oldestId;
  int idValue = _length;
  QMapIterator< T,int > item(_map);

  while (item.hasNext())
  {
    item.next();

    if (idValue > item.value())
    {
      oldestId = item.key();
      idValue = item.value();
    }
  }

  return oldestId;
}

template < typename T > void IdCache< T >::cleanOldest()
{
  if (!_map.size() || (_map.size() < _length))
    return;

  T oldestId = findLowestId();

  _map.remove(oldestId);
  qDebug() << "IdCache: removed" << oldestId;
}

template < typename T > bool IdCache< T >::contains(const T &id) const
{
  bool found = _map.contains(id);

  qDebug() << "IdCache: contains" << id << "=" << found;

  return found;
}

template < typename T > void IdCache< T >::updateCounters(const T &id)
{
  QMutableMapIterator< T,int> item(_map);

  while (item.hasNext())
  {
    item.next();

    if (item.key() != id)
      item.setValue(item.value() - 1);
  }
}

template < typename T > void IdCache< T >::insert(const T &id)
{
  qDebug() << "IdCache: added" << id;

  _map.insert(id,_length);

  updateCounters(id);

  cleanOldest();
}

template < typename T > QDataStream &operator<<(QDataStream &out, const IdCache< T > &cache)
{
  out << cache._map;
  return out;
}

template < typename T > QDataStream &operator>>(QDataStream &in, IdCache< T > &cache)
{
  in >> cache._map;
  return in;
}

#endif
