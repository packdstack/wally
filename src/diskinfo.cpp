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

#include <QtGlobal>

#ifdef Q_WS_WIN
  #include <string.h>
  #include <windows.h>
#else
  #include <sys/statvfs.h>
#endif

#include "diskinfo.h"

qint64 DiskInfo::freeSpace(const QString &fileName)
{
  qint64 result;

#ifdef Q_WS_WIN
  ULARGE_INTEGER winResult, temp1, temp2;

  #ifdef UNICODE
    WCHAR path[1024];

    memset(path,0,1024 * sizeof(WCHAR));
    QDir::toNativeSeparators(QFileInfo(fileName).absolutePath()).toWCharArray(path);
    if (!GetDiskFreeSpaceExW(path,&winResult,&temp1,&temp2))
      return -1;
  #else
    if (!GetDiskFreeSpaceExA(QDir::toNativeSeparators(QFileInfo(fileName).absolutePath()).toAscii().constData(),&winResult,&temp1,&temp2))
      return -1;
  #endif

  result = winResult.QuadPart;
#else
  struct statvfs diskData;

  if (statvfs(QFileInfo(fileName).absoluteDir().rootPath().toAscii().constData(),&diskData) == -1)
    return -1;

  result = static_cast<qint64> (diskData.f_bsize) * static_cast<qint64> (diskData.f_bavail);
#endif

  return result;
}
