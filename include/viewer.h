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

#ifndef VIEWER_H
#define VIEWER_H

#include <QtCore>
#include <QtGui>

#include "gui.h"
#include "ui_viewer.h"

class Viewer : public Gui::Dialog, private Ui::Viewer
{
  Q_OBJECT

  QPixmap _pixmap;
  QMatrix _zoomMatrix;
  QMatrix _rotationMatrix;

private slots:
  void on_actionFitToWindow_triggered();
  void on_actionShowFullImage_triggered();
  void on_actionZoomIn_triggered();
  void on_actionZoomOut_triggered();
  void on_actionRotateClockwise_triggered();
  void on_actionRotateCClockwise_triggered();

protected:
  void showEvent(QShowEvent *event);

public:
  Viewer(QWidget *parent);
  virtual ~Viewer() { }

  void setPixmap(const QPixmap &pixmap);
};

#endif
