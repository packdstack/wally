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

#include <QtGui>
#include <QtDebug>

#include "utils.h"

QMap<QString, QString> getFormats()
{
  QMap<QString, QString> formats;

  formats["bw"] = "Black & White";
  formats["eps"] = "Encapsulated Postscript";
  formats["epsf"] = "Encapsulated PostScript";
  formats["epsi"] = "Encapsulated PostScript Interchange";
  formats["exr"] = "OpenEXR";
  formats["pcx"] = "PC Paintbrush Exchange";
  formats["psd"] = "Photoshop Document";
  formats["rgb"] = "Raw red, green, and blue samples";
  formats["rgba"] = "Raw red, green, blue, and alpha samples";
  formats["sgi"] = "Irix RGB";
  formats["tga"] = "Truevision Targa";
  formats["xcf"] = "eXperimental Computing Facility (GIMP)";
  formats["dds"] = "DirectDraw Surface";
  formats["xv"] = "Khoros Visualization";
  formats["bmp"] = "Windows Bitmap";
  formats["gif"] = "Graphic Interchange Format";
  formats["jpg"] = "Joint Photographic Experts Group";
  formats["jpeg"] = "Joint Photographic Experts Group";
  formats["jp2"] = "Joint Photographic Experts Group 2000";
  formats["mng"] = "Multiple-image Network Graphics";
  formats["png"] = "Portable Network Graphics";
  formats["pbm"] = "Portable Bitmap";
  formats["pgm"] = "Portable Graymap";
  formats["ppm"] = "Portable Pixmap";
  formats["tiff"] = "Tagged Image File Format";
  formats["tif"] = "Tagged Image File Format";
  formats["xbm"] = "X11 Bitmap";
  formats["xpm"] = "X11 Pixmap";
  formats["ico"] = "Icon Image";
  formats["svg"] = "Scalable Vector Graphics";

  return formats;
}

QStringList getImageFilters(bool shortView)
{
  QString global;
  QStringList list;
  QMap<QString, QString> formats = getFormats();
  QStringList supportedFormats = getSupportedImageFormats();
  QStringListIterator supportedFormat(supportedFormats);

  if (shortView)
    while (supportedFormat.hasNext())
      list << "*." + supportedFormat.next();
  else
  {
    list << QObject::tr("All image files") + " (" + getImageFilters(true).join(" ") + ")";

    while (supportedFormat.hasNext())
    {
      list << formats.value(supportedFormat.peekNext()) + " " + QObject::tr("files") +
              " (*." + supportedFormat.peekNext() + ")";
      supportedFormat.next();
    }
  }

  return list;
}

QStringList getSupportedImageFormats()
{
  QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
  QStringList result;
  QListIterator<QByteArray> supportedFormat(supportedFormats);
  QByteArray format;
  QString debugMsg = "getSupportedImageFormats():";

  while (supportedFormat.hasNext())
  {
    format = supportedFormat.next().toLower();

    if (!result.contains(format))
    {
      result << format;
      debugMsg += QString("\t\"") + format.constData() + "\"";
    }
  }

  qDebug() << debugMsg;

  return result;
}
