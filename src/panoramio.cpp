/*
TRANSLATOR Panoramio::Item
*/
/*
TRANSLATOR Panoramio::Core
*/
/*
TRANSLATOR Panoramio::DialogWidget
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

#include "json.h"
#include "gui.h"
#include "httpengine.h"
#include "panoramio.h"
#include "mapviewer.h"

using namespace Panoramio;

Item::Item() : HttpEngine::Item()
{
  _requestedSize = Original;
  _searchOrder = Popularity;
  _minLatitude = _maxLatitude = _minLongitude = _maxLongitude = 0.0;
}

Item::Item(Item *item) : HttpEngine::Item(item)
{
  _requestedSize = item->_requestedSize;
  _searchOrder = item->_searchOrder;
  _minLatitude = item->_minLatitude;
  _maxLatitude = item->_maxLatitude;
  _minLongitude = item->_minLongitude;
  _maxLongitude = item->_maxLongitude;
}

int Item::doCalculateNextIndex(bool randomMode, int currentIndex, int pages) const
{
  return (pages)? (currentIndex + ((randomMode)? (qrand() % 5) + 1 : 1)) % pages : 1;
}

QUrl Item::doPrepareInit(HttpEngine::Operation & /* op */)
{
  _photoId.clear();
  photoOwner.clear();
  photoTitle.clear();
  photoLocation.clear();
  _photoUrl.clear();
  _sourceUrl.clear();
  photoSize = QSize(-1,-1);

  return QUrl();
}

QUrl Item::doPrepareSearch(HttpEngine::Operation & /* op */)
{
  QUrl url("http://www.panoramio.com/map/get_panoramas.php");

  url.addQueryItem("order",searchOrderStrings[_searchOrder]);
  url.addQueryItem("set","public");
  url.addQueryItem("size",requestedSizeStrings[_requestedSize]);
  url.addQueryItem("from",QString::number(pageIndex()-1));
  url.addQueryItem("to",QString::number(pageIndex()));
  url.addQueryItem("minx",QString::number(_minLongitude));
  url.addQueryItem("miny",QString::number(_minLatitude));
  url.addQueryItem("maxx",QString::number(_maxLongitude));
  url.addQueryItem("maxy",QString::number(_maxLatitude));

  return url;
}

bool Item::doProcessSearchResult(const QByteArray &response, int &newPagesCount)
{
  QByteArray resp = response;
  QBuffer buffer(&resp);

  buffer.open(QIODevice::ReadOnly);

  try
  {
    JSON::Reader reader(&buffer);

    JSON::Object *object;
    JSON::Array *array;
    JSON::Value *value, *latitude, *longitude;

    object = dynamic_cast<JSON::Object *> (reader.element());

    if (!object)
    {
      qDebug() << metaObject()->className() << ": cannot find first object";
      return false;
    }

    value = dynamic_cast<JSON::Value *> (object->value("count"));

    if (!value)
    {
      qDebug() << metaObject()->className() << ": cannot find count value";
      return false;
    }

    newPagesCount = value->toInt();

    array = dynamic_cast<JSON::Array *> (object->value("photos"));

    if (!array || !array->size())
    {
      qDebug() << metaObject()->className() << ": cannot find photos array";
      return false;
    }

    object = dynamic_cast<JSON::Object *> (array->at(0));

    if (!object)
    {
      qDebug() << metaObject()->className() << ": cannot find object inside array";
      return false;
    }

    value = dynamic_cast<JSON::Value *> (object->value("photo_id"));
    if (value)
      _photoId = QString::number(value->toInt());
    else
      qDebug() << metaObject()->className() << ": cannot find photo_id";

    value = dynamic_cast<JSON::Value *> (object->value("photo_file_url"));
    if (value)
      _photoUrl = value->toString();
    else
      qDebug() << metaObject()->className() << ": cannot find photo_file_url";

    value = dynamic_cast<JSON::Value *> (object->value("photo_url"));
    if (value)
      _sourceUrl = value->toString();
    else
      qDebug() << metaObject()->className() << ": cannot find photo_url";

    value = dynamic_cast<JSON::Value *> (object->value("photo_title"));
    if (value)
      photoTitle = value->toString();
    else
      qDebug() << metaObject()->className() << ": cannot find photo_title";

    value = dynamic_cast<JSON::Value *> (object->value("owner_name"));
    if (value)
      photoOwner = value->toString();
    else
      qDebug() << metaObject()->className() << ": cannot find owner_name";

    value = dynamic_cast<JSON::Value *> (object->value("width"));
    if (value)
      photoSize.setWidth(value->toInt());
    else
      qDebug() << metaObject()->className() << ": cannot find width";

    value = dynamic_cast<JSON::Value *> (object->value("height"));
    if (value)
      photoSize.setHeight(value->toInt());
    else
      qDebug() << metaObject()->className() << ": cannot find height";

    latitude = dynamic_cast<JSON::Value *> (object->value("latitude"));
    longitude = dynamic_cast<JSON::Value *> (object->value("longitude"));

    if (latitude && longitude)
      photoLocation = tr("lat:") + " " + QString::number(latitude->toDouble()) + ", " +
                      tr("lon:") + " " + QString::number(longitude->toDouble());
    else
      qDebug() << metaObject()->className() << ": cannot find coordinates";
  }
  catch (const QString &message)
  {
    qDebug() << message;
  }

  return (!_photoId.isEmpty() && !_lastPhotoIds.contains(_photoId));
}

QUrl Item::prepareSizeRequest(HttpEngine::Operation & /* op */)
{
  return QUrl("/");
}

QSize Item::doProcessSizeRequestResult(const QByteArray & /* response */)
{
  return photoSize;
}

QUrl Item::prepareDownload(HttpEngine::Operation & /* op */)
{
  if (_requestedSize == Original)
    return QUrl("http://static.panoramio.com/photos/original/" + _photoId + ".jpg");
  else
    return _photoUrl;
}

QUrl Item::prepareInfoCollect(HttpEngine::Operation & /* op */)
{
  _lastPhotoIds.insert(_photoId);
  return QUrl("/");
}

PhotoInfo Item::doProcessInfoCollectResult(const QByteArray & /* response */)
{
  PhotoInfo info;

  info.title = photoTitle;
  info.owner = photoOwner;
  info.location = photoLocation;
  info.sourceUrl = _sourceUrl;
  info.searchString = QString("%1,%2,%3,%4").arg(_minLongitude).arg(_maxLatitude).arg(_maxLongitude).arg(_minLatitude);

  return info;
}

void Item::doLoadSettings(QSettings &settings)
{
  QByteArray data;
  QBuffer buf(&data);
  QDataStream in(&buf);

  _minLongitude = settings.value(MIN_LONGITUDE,-180.0).toDouble();
  _minLatitude = settings.value(MIN_LATITUDE,-90.0).toDouble();
  _maxLongitude = settings.value(MAX_LONGITUDE,180.0).toDouble();
  _maxLatitude = settings.value(MAX_LATITUDE,90.0).toDouble();
  _searchOrder = static_cast<SearchOrder> (settings.value(SEARCH_ORDER,Popularity).toInt());
  _requestedSize = static_cast<RequestedSize> (settings.value(REQUESTED_SIZE,Original).toInt());

  data = qUncompress(settings.value(LAST_PHOTO_IDS,QByteArray()).toByteArray());

  buf.open(QIODevice::ReadOnly);
  in >> _lastPhotoIds;
  buf.close();
}

void Item::doSaveSettings(QSettings &settings) const
{
  settings.setValue(MIN_LONGITUDE,_minLongitude);
  settings.setValue(MIN_LATITUDE,_minLatitude);
  settings.setValue(MAX_LONGITUDE,_maxLongitude);
  settings.setValue(MAX_LATITUDE,_maxLatitude);
  settings.setValue(SEARCH_ORDER,_searchOrder);
  settings.setValue(REQUESTED_SIZE,_requestedSize);
}

void Item::doSaveState(QSettings &settings) const
{
  QByteArray data;
  QBuffer buf(&data);
  QDataStream out(&buf);

  buf.open(QIODevice::WriteOnly);
  out << _lastPhotoIds;
  buf.close();

  settings.setValue(LAST_PHOTO_IDS,qCompress(data));
}

QVariant Item::data() const
{
  QString size, order;

  switch (_requestedSize)
  {
    case Original: size = tr("Original"); break;
    case Medium: size = tr("Medium"); break;
    case Small: size = tr("Small"); break;
    case Thumbnail: size = tr("Thumbnail"); break;
    case Square: size = tr("Square"); break;
    case MiniSquare: size = tr("Mini square"); break;
  }

  switch (_searchOrder)
  {
    case Popularity: order = tr("Popularity"); break;
    case UploadDate: order = tr("Upload date"); break;
  }

  return QString("<font size=\"+1\"><table cellspacing=\"3\"><tr><td><b>") + tr("Longitude:") +
         "</b></td><td align=\"right\">" + QString::number(minLongitude(),'f') + "</td>" +
         "<td align=\"center\">-></td><td align=\"right\">" + QString::number(maxLongitude(),'f') +
         "</td></tr><tr><td><b>" + tr("Latitude:") + "</b></td><td align=\"right\">" +
         QString::number(minLatitude(),'f') + "</td><td align=\"center\">-></td><td align=\"right\">" +
         QString::number(maxLatitude(),'f') + "</td></tr></table>" +
         "</font><i>&nbsp;&nbsp;" + tr("Size:") + "&nbsp;" + size + ",&nbsp;&nbsp;" +
         tr("Order:") + "&nbsp;" + order + "</i>";
}

bool Item::equalTo(Engine::Item *item) const
{
  Item *panoramioItem = qobject_cast<Item *> (item);

  return panoramioItem && (minLatitude() == panoramioItem->minLatitude()) &&
                          (maxLatitude() == panoramioItem->maxLatitude()) &&
                          (minLongitude() == panoramioItem->minLongitude()) &&
                          (maxLongitude() == panoramioItem->maxLongitude());
}

DialogWidget::DialogWidget(XtEngine::Core *core, QWidget *parent) : XtEngine::DialogWidget(core,parent)
{
  QVBoxLayout *mainLayout = new QVBoxLayout;
  QFormLayout *formLayout = new QFormLayout;
  QPushButton *pbMap = new QPushButton(tr("Select on map"),this);

  checkingValues = false;

  connect(pbMap,SIGNAL(clicked()),this,SLOT(launchMapViewer()));

  formLayout->addRow(new QLabel(tr("Longitude (min):"),this),
                     dsbMinLongitude = new QDoubleSpinBox(this));
  formLayout->addRow(new QLabel(tr("Longitude (max):"),this),
                     dsbMaxLongitude = new QDoubleSpinBox(this));
  formLayout->addRow(new QLabel(tr("Latitude (min):"),this),
                     dsbMinLatitude = new QDoubleSpinBox(this));
  formLayout->addRow(new QLabel(tr("Latitude (max):"),this),
                     dsbMaxLatitude = new QDoubleSpinBox(this));

  dsbMaxLongitude->setRange(-180.0,180.0);
  dsbMaxLongitude->setDecimals(6);
  connect(dsbMaxLongitude,SIGNAL(editingFinished()),this,SLOT(correctValues()));
  dsbMinLongitude->setRange(-180.0,180.0);
  dsbMinLongitude->setDecimals(6);
  connect(dsbMinLongitude,SIGNAL(editingFinished()),this,SLOT(correctValues()));
  dsbMaxLatitude->setRange(-90.0,90.0);
  dsbMaxLatitude->setDecimals(6);
  connect(dsbMaxLatitude,SIGNAL(editingFinished()),this,SLOT(correctValues()));
  dsbMinLatitude->setRange(-90.0,90.0);
  dsbMinLatitude->setDecimals(6);
  connect(dsbMinLatitude,SIGNAL(editingFinished()),this,SLOT(correctValues()));

  cbSearchOrder = new QComboBox(this);
  cbSearchOrder->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbSearchOrder->addItem(tr("Popularity"),Popularity);
  cbSearchOrder->addItem(tr("Upload date"),UploadDate);
  cbSearchOrder->setCurrentIndex(0);

  cbRequestedSize = new QComboBox(this);
  cbRequestedSize->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbRequestedSize->addItem(tr("Original"),Original);
  cbRequestedSize->addItem(tr("Medium"),Medium);
  cbRequestedSize->addItem(tr("Small"),Small);
  cbRequestedSize->addItem(tr("Thumbnail"),Thumbnail);
  cbRequestedSize->addItem(tr("Square"),Square);
  cbRequestedSize->addItem(tr("Mini square"),MiniSquare);
  cbRequestedSize->setCurrentIndex(0);

  formLayout->addRow(new QLabel(tr("Order:"),this),cbSearchOrder);
  formLayout->addRow(new QLabel(tr("Size:"),this),cbRequestedSize);

  mainLayout->addWidget(pbMap);
  mainLayout->addLayout(formLayout);

  setLayout(mainLayout);

  dsbMinLongitude->setFocus();
}

void DialogWidget::correctValues()
{
  if (!checkingValues)
  {
    checkingValues = true;

    dsbMaxLongitude->setValue(qMax(dsbMinLongitude->value(),dsbMaxLongitude->value()));
    dsbMinLongitude->setValue(qMin(dsbMinLongitude->value(),dsbMaxLongitude->value()));
    dsbMaxLatitude->setValue(qMax(dsbMinLatitude->value(),dsbMaxLatitude->value()));
    dsbMinLatitude->setValue(qMin(dsbMinLatitude->value(),dsbMaxLatitude->value()));

    checkingValues = false;
  }
}

XtEngine::Item *DialogWidget::returnItem()
{
  Item *item = qobject_cast<Item *> (core()->newItem());

  item->setMinLongitude(dsbMinLongitude->value());
  item->setMaxLongitude(dsbMaxLongitude->value());
  item->setMinLatitude(dsbMinLatitude->value());
  item->setMaxLatitude(dsbMaxLatitude->value());
  item->setSearchOrder(static_cast<SearchOrder>
                         (cbSearchOrder->itemData(cbSearchOrder->currentIndex()).toInt()));
  item->setRequestedSize(static_cast<RequestedSize>
                          (cbRequestedSize->itemData(cbRequestedSize->currentIndex()).toInt()));

  return item;
}

void DialogWidget::setupFromItem(XtEngine::Item *item)
{
  if (item)
  {
    Item *panoramioItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit Panoramio item"));

    checkingValues = true;
    dsbMinLatitude->setValue(panoramioItem->minLatitude());
    dsbMaxLatitude->setValue(panoramioItem->maxLatitude());
    dsbMinLongitude->setValue(panoramioItem->minLongitude());
    dsbMaxLongitude->setValue(panoramioItem->maxLongitude());
    checkingValues = false;

    cbSearchOrder->setCurrentIndex(cbSearchOrder->findData(panoramioItem->searchOrder()));
    cbRequestedSize->setCurrentIndex(cbRequestedSize->findData(panoramioItem->requestedSize()));
  }
  else
    setWindowTitle(tr("Add Panoramio item"));
}

bool DialogWidget::validateInput()
{
  if ((dsbMinLatitude->value() == dsbMaxLatitude->value()) ||
      (dsbMinLongitude->value() == dsbMaxLongitude->value()))
  {
    QMessageBox::critical(this,tr("Error"),tr("Coordinates must be different"));
    return false;
  }
  else
    return true;
}

void DialogWidget::launchMapViewer()
{
  Map::Viewer *viewer;

  if ((dsbMaxLongitude->value() > dsbMinLongitude->value()) &&
      (dsbMaxLatitude->value() > dsbMinLatitude->value()))
    viewer = new Map::Viewer(QRectF(QPointF(dsbMinLongitude->value(),
                                            dsbMaxLatitude->value()),
                                    QPointF(dsbMaxLongitude->value(),
                                            dsbMinLatitude->value())),this);
  else
    viewer = new Map::Viewer(this);

  if (viewer->exec() == QDialog::Accepted)
  {
    QRectF llRect = viewer->llRect();

    checkingValues = true;
    dsbMaxLongitude->setValue(llRect.right());
    dsbMinLongitude->setValue(llRect.left());
    dsbMaxLatitude->setValue(llRect.top());
    dsbMinLatitude->setValue(llRect.bottom());
    checkingValues = false;
  }

  delete viewer;
}
