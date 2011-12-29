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

#ifndef MAPVIEWER_H
#define MAPVIEWER_H

#include <QtCore>
#include <QtGui>

#include "gui.h"

#define K 2.30341254338
#define ANCHOR_HALF_LENGTH 4

namespace Map
{
  class AnchorRect : public QObject, public QGraphicsRectItem
  {
    Q_OBJECT

  public:
    enum Position { noAnchor, topLeft, top, topRight, right, bottomRight, bottom, bottomLeft, left };

  private:
    Position _position;

  protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

  public:
    AnchorRect(Position position, QGraphicsItem *parent = 0);
    virtual ~AnchorRect() { }

  signals:
    void selected(AnchorRect::Position anchor);
  };

  class SelectionRect : public QObject, public QGraphicsRectItem
  {
    Q_OBJECT

    QMap<AnchorRect::Position, AnchorRect *> rects;

    QPointF startP;
    QPointF startMouseP;
    QRectF startRect;
    AnchorRect::Position _selectedAnchor;
    void setupRect();

    void updateAnchorsPosition();

  private slots:
    void onAnchorSelected(AnchorRect::Position anchor);

  protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

  public:
    SelectionRect(QGraphicsItem *parent = 0);
    SelectionRect(const QRectF &rect, QGraphicsItem *parent = 0);
    SelectionRect(qreal x, qreal y, qreal width, qreal height, QGraphicsItem *parent = 0);
    virtual ~SelectionRect() { }

    void setPos(const QPointF &pos);
    void setPos(qreal x, qreal y);
    void setRect(const QRectF &rectangle);
    void setRect(qreal x, qreal y, qreal width, qreal height);

    QRectF boundingRect() const;
  };

  class MapItem : public QGraphicsPixmapItem
  {
    bool drawingRect;
    QPointF startP;
    SelectionRect *rectItem;

    void setupGrid();

  protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

  public:
    MapItem(QGraphicsItem *parent = 0);
    MapItem(const QPixmap &pixmap, QGraphicsItem *parent = 0);
    virtual ~MapItem() { }

    void setLLRect(const QRectF &llRect);
    QRectF llRect() const;

    QPainterPath shape() const;
  };

  class Scene : public QGraphicsScene
  {
    Q_OBJECT

    MapItem *mapItem;
    QPointF rectPos;

  public:
    Scene(QObject *parent = 0);
    virtual ~Scene() { }

    QSizeF mapSize() const { return mapItem->boundingRect().size(); }

    void setLLRect(const QRectF &llRect) { mapItem->setLLRect(llRect); }
    QRectF llRect() const { return mapItem->llRect(); }
  };

  class View : public QGraphicsView
  {
    Q_OBJECT

    QSignalMapper *signalMapper;
    int currentZoom;

    QAction *newZoomAction(int zoomFactor);

  private slots:
    void zoom(int zoomFactor);

  public:
    View(QWidget *parent = 0);
    View(QGraphicsScene *scene, QWidget *parent = 0);
    virtual ~View() { }

    QSize minimumSizeHint() const { return sizeHint(); }
    QSize sizeHint() const
      { return qobject_cast<Scene *> (scene())->mapSize().toSize() +
               QSize(verticalScrollBar()->width() + 2 * ANCHOR_HALF_LENGTH,
                     horizontalScrollBar()->height() + 2 * ANCHOR_HALF_LENGTH); }
  };

  class Viewer : public Gui::Dialog
  {
    Q_OBJECT

    Scene *scene;

    void setupUI();

  public:
    Viewer(QWidget *parent = 0);
    Viewer(const QRectF &llRect, QWidget *parent = 0);
    virtual ~Viewer() { }

    QRectF llRect() const { return scene->llRect(); }

    QSize sizeHint() const { return scene->mapSize().toSize(); }
  };
}

#endif
