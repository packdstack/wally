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

#ifndef VERSION_H
#define VERSION_H

#include <QString>
#include <QStringList>

class Version
{
  int _majorNumber;
  int _minorNumber;
  int _patchLevelNumber;

public:
  Version(int majorNumber, int minorNumber, int patchLevelNumber) :
    _majorNumber(majorNumber), _minorNumber(minorNumber), _patchLevelNumber(patchLevelNumber) { }
  Version(const QString &versionStr)
  {
    QStringList versionParts = versionStr.split(".");

    _majorNumber = _minorNumber = _patchLevelNumber = 0;

    if (versionParts.size() > 0)
      _majorNumber = versionParts.at(0).toInt();

    if (versionParts.size() > 1)
      _minorNumber = versionParts.at(1).toInt();

    if (versionParts.size() > 2)
      _patchLevelNumber = versionParts.at(2).toInt();
  }

  int majorNumber() const { return _majorNumber; }
  int minorNumber() const { return _minorNumber; }
  int patchLevelNumber() const { return _patchLevelNumber; }

  QString toString() const
    { return QString("%1.%2.%3").arg(majorNumber()).arg(minorNumber()).arg(patchLevelNumber()); }

  bool operator<(const Version &version)
  {
    return (majorNumber() < version.majorNumber()) ||
           ((majorNumber() == version.majorNumber()) &&
            (minorNumber() < version.minorNumber())) ||
           ((majorNumber() == version.majorNumber()) &&
            (minorNumber() == version.minorNumber()) &&
            (patchLevelNumber() < version.patchLevelNumber()));
  }

  bool operator<=(const Version &version)
  {
    return (*this < version) || (*this == version);
  }

  bool operator>(const Version &version)
  {
    return !(*this <= version);
  }

  bool operator>=(const Version &version)
  {
    return !(*this < version);
  }

  bool operator==(const Version &version)
  {
    return (majorNumber() == version.majorNumber()) &&
           (minorNumber() == version.minorNumber()) &&
           (patchLevelNumber() == version.patchLevelNumber());
  }

  bool operator!=(const Version &version)
  {
    return !(*this == version);
  }
};

#endif
