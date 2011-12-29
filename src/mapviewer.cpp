/*
TRANSLATOR Map::View
*/
/*
TRANSLATOR Map::Viewer
*/

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

#include <math.h>
#include <QtDebug>

#include "gui.h"
#include "mapviewer.h"

using namespace Map;

AnchorRect::AnchorRect(AnchorRect::Position position, QGraphicsItem *parent) : QObject(), QGraphicsRectItem(parent)
{
  _position = position;

  setAcceptHoverEvents(true);
  setCacheMode(QGraphicsItem::NoCache);
  setPen(QPen(Qt::white,1));
  if (parentItem())
    setZValue(parentItem()->zValue() + 0.1);
  setRect(QRectF(QPointF(-ANCHOR_HALF_LENGTH,-ANCHOR_HALF_LENGTH),
                 QPointF(ANCHOR_HALF_LENGTH,ANCHOR_HALF_LENGTH)));

  switch (_position)
  {
    case topLeft:
    case bottomRight:
      setCursor(QCursor(Qt::SizeFDiagCursor));
      break;

    case top:
    case bottom:
      setCursor(QCursor(Qt::SizeVerCursor));
      break;

    case left:
    case right:
      setCursor(QCursor(Qt::SizeHorCursor));
      break;

    case topRight:
    case bottomLeft:
      setCursor(QCursor(Qt::SizeBDiagCursor));
      break;

    default:
      break;
  }
}

void AnchorRect::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
  emit selected(_position);
  QGraphicsRectItem::hoverEnterEvent(event);
}

void AnchorRect::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
  emit selected(noAnchor);
  QGraphicsRectItem::hoverLeaveEvent(event);
}

SelectionRect::SelectionRect(QGraphicsItem *parent) : QObject(), QGraphicsRectItem(parent)
{
  setupRect();
}

SelectionRect::SelectionRect(const QRectF &rect, QGraphicsItem *parent) : QObject(), QGraphicsRectItem(rect,parent)
{
  setupRect();
}

SelectionRect::SelectionRect(qreal x, qreal y, qreal width, qreal height, QGraphicsItem *parent) : QObject(), QGraphicsRectItem(x,y,width,height,parent)
{
  setupRect();
}

void SelectionRect::setupRect()
{
  int anchorPosition;

  _selectedAnchor = AnchorRect::noAnchor;

  setPen(QPen(Qt::white,1,Qt::DashLine));
  setAcceptedMouseButtons(Qt::LeftButton);
  setCacheMode(QGraphicsItem::NoCache);
  if (parentItem())
    setZValue(parentItem()->zValue() + 0.1);
  setCursor(QCursor(Qt::SizeAllCursor));

  rects[AnchorRect::topLeft] = new AnchorRect(AnchorRect::topLeft,this);
  rects[AnchorRect::top] = new AnchorRect(AnchorRect::top,this);
  rects[AnchorRect::topRight] = new AnchorRect(AnchorRect::topRight,this);
  rects[AnchorRect::right] = new AnchorRect(AnchorRect::right,this);
  rects[AnchorRect::bottomRight] = new AnchorRect(AnchorRect::bottomRight,this);
  rects[AnchorRect::bottom] = new AnchorRect(AnchorRect::bottom,this);
  rects[AnchorRect::bottomLeft] = new AnchorRect(AnchorRect::bottomLeft,this);
  rects[AnchorRect::left] = new AnchorRect(AnchorRect::left,this);

  updateAnchorsPosition();

  for (anchorPosition = static_cast<int> (AnchorRect::topLeft);
       anchorPosition <= static_cast<int> (AnchorRect::left); ++anchorPosition)
    connect(rects.value(static_cast<AnchorRect::Position> (anchorPosition)),
            SIGNAL(selected(AnchorRect::Position)),
            this,SLOT(onAnchorSelected(AnchorRect::Position)));
}

void SelectionRect::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  switch (_selectedAnchor)
  {
    case AnchorRect::topLeft:
      setRect(startRect.adjusted((mapToParent(event->pos()) - startMouseP).x(),
                                 (mapToParent(event->pos()) - startMouseP).y(),0,0));
      break;

    case AnchorRect::bottomRight:
      setRect(startRect.adjusted(0,0,(mapToParent(event->pos()) - startMouseP).x(),
                                 (mapToParent(event->pos()) - startMouseP).y()));
      break;

    case AnchorRect::top:
      setRect(startRect.adjusted(0,(mapToParent(event->pos()) - startMouseP).y(),0,0));
      break;

    case AnchorRect::bottom:
      setRect(startRect.adjusted(0,0,0,(mapToParent(event->pos()) - startMouseP).y()));
      break;

    case AnchorRect::left:
      setRect(startRect.adjusted((mapToParent(event->pos()) - startMouseP).x(),0,0,0));
      break;

    case AnchorRect::right:
      setRect(startRect.adjusted(0,0,(mapToParent(event->pos()) - startMouseP).x(),0));
      break;

    case AnchorRect::topRight:
      setRect(startRect.adjusted(0,(mapToParent(event->pos()) - startMouseP).y(),
                                 (mapToParent(event->pos()) - startMouseP).x(),0));
      break;

    case AnchorRect::bottomLeft:
      setRect(startRect.adjusted((mapToParent(event->pos()) - startMouseP).x(),0,
                                 0,(mapToParent(event->pos()) - startMouseP).y()));
      break;

    case AnchorRect::noAnchor:
      setPos(startP + mapToParent(event->pos()) - startMouseP);
      break;
  }

  updateAnchorsPosition();
}

void SelectionRect::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsRectItem::mousePressEvent(event);

  startP = pos();
  startMouseP = mapToParent(event->pos());
  startRect = rect();

  event->accept();
}

void SelectionRect::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  setRect(rect().normalized());
  updateAnchorsPosition();
  QGraphicsRectItem::mouseReleaseEvent(event);
}

void SelectionRect::updateAnchorsPosition()
{
  rects[AnchorRect::topLeft]->setPos(rect().topLeft());
  rects[AnchorRect::top]->setPos(QPointF(rect().center().x(),rect().top()));
  rects[AnchorRect::topRight]->setPos(rect().topRight());
  rects[AnchorRect::right]->setPos(QPointF(rect().right(),rect().center().y()));
  rects[AnchorRect::bottomRight]->setPos(rect().bottomRight());
  rects[AnchorRect::bottom]->setPos(QPointF(rect().center().x(),rect().bottom()));
  rects[AnchorRect::bottomLeft]->setPos(rect().bottomLeft());
  rects[AnchorRect::left]->setPos(QPointF(rect().left(),rect().center().y()));
}

void SelectionRect::onAnchorSelected(AnchorRect::Position anchor)
{
  _selectedAnchor = anchor;
}

QRectF SelectionRect::boundingRect() const
{
  return QGraphicsRectItem::boundingRect().adjusted(-ANCHOR_HALF_LENGTH,-ANCHOR_HALF_LENGTH,
                                                    ANCHOR_HALF_LENGTH,ANCHOR_HALF_LENGTH);
}

void SelectionRect::setPos(const QPointF &pos)
{
  QGraphicsRectItem::setPos(pos);
  updateAnchorsPosition();
}

void SelectionRect::setPos(qreal x, qreal y)
{
  setPos(QPointF(x,y));
}

void SelectionRect::setRect(const QRectF &rectangle)
{
  QGraphicsRectItem::setRect(rectangle);
  updateAnchorsPosition();
}

void SelectionRect::setRect(qreal x, qreal y, qreal width, qreal height)
{
  setRect(QRectF(x,y,width,height));
}

MapItem::MapItem(QGraphicsItem *parent) : QGraphicsPixmapItem(parent)
{
  rectItem = 0;
  drawingRect = false;
  setAcceptedMouseButtons(Qt::LeftButton);
  setupGrid();
}

MapItem::MapItem(const QPixmap &pixmap, QGraphicsItem *parent) : QGraphicsPixmapItem(pixmap,parent)
{
  rectItem = 0;
  drawingRect = false;
  setAcceptedMouseButtons(Qt::LeftButton);
  setupGrid();
}

void MapItem::setupGrid()
{
  if (!pixmap().size().isEmpty())
  {
    int i;

    for (i = -180; i <= 180; i += 30)
    {
      QGraphicsLineItem *line;
      qreal x = static_cast<double> (pixmap().width()) / 360.0 * (static_cast<double> (i) + 180.0);

      line = new QGraphicsLineItem(QLineF(QPointF(x,0),QPointF(x,pixmap().height())),this);
      line->setPen(QPen(Qt::darkGray,1,Qt::DotLine));
    }

    for (i = -90; i <= 90; i += 30)
    {
      QGraphicsLineItem *line;
      qreal y = (K - 1.25 * log(tan(0.25 * M_PI + 0.2 * static_cast<double> (i) * M_PI / 90.0))) / (2 * K) * static_cast<double> (pixmap().height());

      line = new QGraphicsLineItem(QLineF(QPointF(0,y),QPointF(pixmap().width(),y)),this);
      line->setPen(QPen(Qt::darkGray,1,Qt::DotLine));
    }
  }
}

void MapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  if (drawingRect)
  {
    rectItem->setRect(QRectF(QPointF(0,0),event->pos() - startP));
    event->accept();
  }
  else
    QGraphicsPixmapItem::mouseMoveEvent(event);
}

void MapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  startP = event->pos();
  drawingRect = true;

  if (!rectItem)
    rectItem = new SelectionRect(this);

  rectItem->setPos(startP);
  rectItem->setRect(QRectF());

  event->accept();
}

void MapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  drawingRect = false;
  rectItem->setRect(rectItem->rect().normalized());
  QGraphicsPixmapItem::mouseReleaseEvent(event);
}

QRectF MapItem::llRect() const
{
  QRectF result, selRect = rectItem->mapRectToParent(rectItem->rect()).intersected(boundingRect());

  result.setLeft(360.0 / boundingRect().width() * selRect.topLeft().x() - 180.0);
  result.setRight(360.0 / boundingRect().width() * selRect.bottomRight().x() - 180.0);
  result.setTop(450.0 * (atan(exp(0.8 * K - 1.6 * K * selRect.topLeft().y() / boundingRect().height())) / M_PI - 0.25));
  result.setBottom(450.0 * (atan(exp(0.8 * K - 1.6 * K * selRect.bottomRight().y() / boundingRect().height())) / M_PI - 0.25));

  return result;
}

void MapItem::setLLRect(const QRectF &llRect)
{
  if (!rectItem)
    rectItem = new SelectionRect(this);

  qreal x1 = boundingRect().width() / 360.0 * (llRect.left() + 180.0);
  qreal x2 = boundingRect().width() / 360.0 * (llRect.right() + 180.0);
  qreal y1 = (K - 1.25 * log(tan(0.25 * M_PI + 0.2 * llRect.top() * M_PI / 90.0))) / (2 * K) * boundingRect().height();
  qreal y2 = (K - 1.25 * log(tan(0.25 * M_PI + 0.2 * llRect.bottom() * M_PI / 90.0))) / (2 * K) * boundingRect().height();

  rectItem->setRect(QRectF(QPointF(x1,y1),QPointF(x2,y2)));
}

QPainterPath MapItem::shape() const
{
  QPainterPath path;

  path.addRect(boundingRect().adjusted(-1,-1,1,1));
  return path;
}

Scene::Scene(QObject *parent) : QGraphicsScene(parent)
{
  mapItem = new MapItem(QPixmap(":/images/world_map"),0);
  mapItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape,true);
  mapItem->setAcceptHoverEvents(false);
  mapItem->setHandlesChildEvents(false);
  addItem(mapItem);

  setSceneRect(mapItem->boundingRect().adjusted(-ANCHOR_HALF_LENGTH,-ANCHOR_HALF_LENGTH,
                                                ANCHOR_HALF_LENGTH,ANCHOR_HALF_LENGTH));
}

View::View(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene,parent)
{
  QPalette myPalette = palette();
  QActionGroup *actionGroup = new QActionGroup(this);
  QAction *defaultAction;

  myPalette.setColor(QPalette::Base,Qt::black);
  setPalette(myPalette);

  currentZoom = 1;

  signalMapper = new QSignalMapper(this);
  connect(signalMapper,SIGNAL(mapped(int)),this,SLOT(zoom(int)));

  actionGroup->setExclusive(true);
  actionGroup->addAction(defaultAction = newZoomAction(1));
  actionGroup->addAction(newZoomAction(2));
  actionGroup->addAction(newZoomAction(3));
  actionGroup->addAction(newZoomAction(4));
  defaultAction->setChecked(true);

  setContextMenuPolicy(Qt::ActionsContextMenu);
  setRenderHints(QPainter::HighQualityAntialiasing |
                 QPainter::SmoothPixmapTransform);

  setToolTip(tr("Drag to select. Right-click to zoom"));
  setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);

  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}

QAction *View::newZoomAction(int zoomFactor)
{
  QAction *action = new QAction(tr("Zoom %1x").arg(zoomFactor),this);
  action->setCheckable(true);
  signalMapper->setMapping(action,zoomFactor);
  connect(action,SIGNAL(triggered()),signalMapper,SLOT(map()));
  addAction(action);

  return action;
}

void View::zoom(int zoomFactor)
{
  scale(static_cast<qreal> (zoomFactor) / static_cast<qreal> (currentZoom),
        static_cast<qreal> (zoomFactor) / static_cast<qreal> (currentZoom));
  currentZoom = zoomFactor;
}

Viewer::Viewer(QWidget *parent) : Gui::Dialog(Gui::Dialog::CenterOfScreen,parent)
{
  setupUI();
}

Viewer::Viewer(const QRectF &llRect, QWidget *parent) : Gui::Dialog(Gui::Dialog::CenterOfScreen,parent)
{
  setupUI();

  scene->setLLRect(llRect);
}

void Viewer::setupUI()
{
  QVBoxLayout *layout = new QVBoxLayout;
  scene = new Scene(this);
  View *view = new View(scene,this);
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                     Qt::Horizontal,this);

  connect(buttonBox,SIGNAL(accepted()),this,SLOT(accept()));
  connect(buttonBox,SIGNAL(rejected()),this,SLOT(reject()));

  setWindowTitle(tr("Map viewer"));

  layout->addWidget(view);
  layout->addWidget(buttonBox);

  setLayout(layout);
}
