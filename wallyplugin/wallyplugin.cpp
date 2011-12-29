/*
 * WallyPlugin - KDE4 plugin for Wally
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

#include <QImageReader>
#include <QDBusConnection>
#include <QPointF>

#include "wallyplugin.h"
#include "wallypluginexport.h"

WallyPlugin::WallyPlugin(QObject *parent, const QVariantList &args)
  : Plasma::Wallpaper(parent,args)
{
  QDBusConnection::sessionBus().registerService("com.BeCrux.WallyPlugin");
  QDBusConnection::sessionBus().registerObject("/WallyPlugin",this,QDBusConnection::ExportAllSlots);
}

void WallyPlugin::paint(QPainter *painter, const QRectF &exposedRect)
{
  painter->save();

  if (painter->worldMatrix() == QMatrix())
    painter->resetTransform();

  painter->setCompositionMode(QPainter::CompositionMode_Source);
  if (!_pixmap.isNull())
  {
    if (isPreviewing())
    {
      QPixmap scaledPixmap = _pixmap;
      scaledPixmap = scaledPixmap.scaled(exposedRect.size().toSize(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
      painter->drawPixmap(exposedRect.toRect(),scaledPixmap);
    }
    else
      painter->drawPixmap(exposedRect,_pixmap,exposedRect);
  }
  else
    painter->fillRect(exposedRect,Qt::black);

  painter->restore();
}

void WallyPlugin::loadWallpaper()
{
  if (!_wallpaper.isEmpty())
  {
    _pixmap.load(_wallpaper);
    emit update(boundingRect());
    emit configNeedsSaving();
  }
}

void WallyPlugin::init(const KConfigGroup &config)
{
  Plasma::Wallpaper::init(config);
  setWallpaper(config.readEntry("wallpaper",QString()));
}

void WallyPlugin::save(KConfigGroup &config)
{
  Plasma::Wallpaper::save(config);
  config.writeEntry("wallpaper",_wallpaper);
}

void WallyPlugin::setWallpaper(const QString &fileName)
{
  if (!fileName.isEmpty() && QImageReader(fileName).canRead())
  {
    _wallpaper = fileName;
    loadWallpaper();
  }
}

#include "wallyplugin.moc"
