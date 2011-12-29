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

#include "gui.h"
#include "viewer.h"

Viewer::Viewer(QWidget *parent) : Gui::Dialog(Gui::Dialog::CenterOfScreen,parent)
{
  setupUi(this);

  scrollAreaWidgetContents->setStyleSheet("background-color: rgb(0,0,0);");

  addAction(actionFitToWindow);
  addAction(actionShowFullImage);
  addAction(actionZoomIn);
  addAction(actionZoomOut);
  addAction(actionRotateClockwise);
  addAction(actionRotateCClockwise);

#ifdef Q_WS_MAC
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok,Qt::Horizontal,this);

  connect(buttonBox,SIGNAL(accepted()),this,SLOT(close()));

  layout()->addWidget(buttonBox);
#endif

  resize(qApp->desktop()->availableGeometry().size() - QSize(100,100));
}

void Viewer::setPixmap(const QPixmap &pixmap)
{
  _pixmap = pixmap;
}

void Viewer::showEvent(QShowEvent *event)
{
  QTimer::singleShot(0,actionFitToWindow,SLOT(trigger()));
  Gui::Dialog::showEvent(event);
}

void Viewer::on_actionFitToWindow_triggered()
{
  QPixmap _rotatedPixmap = _pixmap.transformed(_rotationMatrix,Qt::SmoothTransformation);
  _zoomMatrix = QMatrix();

  if ((_rotatedPixmap.size().width() > scrollArea->maximumViewportSize().width()) &&
      (_rotatedPixmap.size().height() > scrollArea->maximumViewportSize().height()))
  {
    qreal hZoomMatrix = static_cast<qreal> (scrollArea->maximumViewportSize().width()) / static_cast<qreal> (_rotatedPixmap.size().width());
    qreal vZoomMatrix = static_cast<qreal> (scrollArea->maximumViewportSize().height()) / static_cast<qreal> (_rotatedPixmap.size().height());
    _zoomMatrix.scale(qMin(hZoomMatrix,vZoomMatrix),qMin(hZoomMatrix,vZoomMatrix));
  }
  else if (_rotatedPixmap.size().width() > scrollArea->maximumViewportSize().width())
    _zoomMatrix.scale(static_cast<qreal> (scrollArea->maximumViewportSize().width()) / static_cast<qreal> (_rotatedPixmap.size().width()),
                      static_cast<qreal> (scrollArea->maximumViewportSize().width()) / static_cast<qreal> (_rotatedPixmap.size().width()));
  else if (_rotatedPixmap.size().height() > scrollArea->maximumViewportSize().height())
    _zoomMatrix.scale(static_cast<qreal> (scrollArea->maximumViewportSize().height()) / static_cast<qreal> (_rotatedPixmap.size().height()),
                      static_cast<qreal> (scrollArea->maximumViewportSize().height()) / static_cast<qreal> (_rotatedPixmap.size().height()));
  else
  {
    qreal hZoomMatrix = static_cast<qreal> (scrollArea->maximumViewportSize().width()) / static_cast<qreal> (_rotatedPixmap.size().width());
    qreal vZoomMatrix = static_cast<qreal> (scrollArea->maximumViewportSize().height()) / static_cast<qreal> (_rotatedPixmap.size().height());
    _zoomMatrix.scale(qMin(hZoomMatrix,vZoomMatrix),qMin(hZoomMatrix,vZoomMatrix));
  }

  label->setPixmap(_rotatedPixmap.transformed(_zoomMatrix,Qt::SmoothTransformation));
}

void Viewer::on_actionShowFullImage_triggered()
{
  _zoomMatrix = QMatrix();
  label->setPixmap(_pixmap.transformed(_zoomMatrix * _rotationMatrix,Qt::SmoothTransformation));
}

void Viewer::on_actionZoomIn_triggered()
{
  _zoomMatrix.scale(1.25,1.25);
  label->setPixmap(_pixmap.transformed(_zoomMatrix * _rotationMatrix,Qt::SmoothTransformation));
}

void Viewer::on_actionZoomOut_triggered()
{
  _zoomMatrix.scale(0.8,0.8);
  label->setPixmap(_pixmap.transformed(_zoomMatrix * _rotationMatrix,Qt::SmoothTransformation));
}

void Viewer::on_actionRotateClockwise_triggered()
{
  _rotationMatrix.rotate(90);
  label->setPixmap(_pixmap.transformed(_zoomMatrix * _rotationMatrix,Qt::SmoothTransformation));
}

void Viewer::on_actionRotateCClockwise_triggered()
{
  _rotationMatrix.rotate(-90);
  label->setPixmap(_pixmap.transformed(_zoomMatrix * _rotationMatrix,Qt::SmoothTransformation));
}
