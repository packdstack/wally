/*
TRANSLATOR Vladstudio::Item
*/
/*
TRANSLATOR Vladstudio::Core
*/
/*
TRANSLATOR Vladstudio::DialogWidget
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

#include <QtXml>

#include "httpengine.h"
#include "vladstudio.h"

using namespace Vladstudio;

QString Item::categoryToString(Category category)
{
  switch (category)
  {
    case All:
      return tr("All categories");
      break;

    case AbstractArt:
      return tr("Abstract art");
      break;

    case Creatures:
      return tr("Creatures");
      break;

    case Illustrations:
      return tr("Illustrations");
      break;

    case Photos:
      return tr("Photos");
      break;

    default:
      return QString();
  }
}

Item::Item() : HttpEngine::Item(), _lastPhotoIds(5)
{
  _category = All;
  _viewOrder = ById;
  _orderDirection = Ascending;
}

Item::Item(Item *item) : HttpEngine::Item(item), _lastPhotoIds(5)
{
  _category = item->category();
  _viewOrder = item->viewOrder();
  _orderDirection = item->orderDirection();
}

QVariant Item::data() const
{
  QString str("<font size=\"+1\"><b>");

  str += categoryToString(_category) + "</b></font> - <i>";

  switch (_viewOrder)
  {
    case ById:
      str += tr("by Id");
      break;

    case ByView:
      str += tr("by view count");
      break;

    default:
      break;
  }

  str += ", ";

  switch (_orderDirection)
  {
    case Ascending:
      str += tr("ascending");
      break;

    case Descending:
      str += tr("descending");
      break;

    default:
      break;
  }

  str += "</i>";

  return str;
}

int Item::doCalculateNextIndex(bool randomMode, int currentIndex, int pages) const
{
  return (pages)? (currentIndex + ((randomMode)? (qrand() % 5) + 1 : 1) % pages) : 0;
}

QUrl Item::doPrepareInit(HttpEngine::Operation & /* op */)
{
  _photoId.clear();
  photoTitle.clear();
  photoDescription.clear();
  _photoUrl.clear();
  _sourceUrl.clear();
  photoOwner.clear();
  photoSize = QSize(-1,-1);

  return QUrl();
}

QUrl Item::doPrepareSearch(HttpEngine::Operation & /* op */)
{
  QUrl url("http://vladstudio.com/api/wally/getlistofwallpapers.php");

  if (category() != All)
    url.addQueryItem("categoryID",QString::number(static_cast< int >(category())));

  switch (viewOrder())
  {
    case ById:
      url.addQueryItem("order","id");
      break;

    case ByView:
      url.addQueryItem("order","viewed");
      break;

    default:
      break;
  }

  switch (orderDirection())
  {
    case Ascending:
      url.addQueryItem("orderdir","asc");
      break;

    case Descending:
      url.addQueryItem("orderdir","desc");
      break;

    default:
      break;
  }

  url.addQueryItem("show","1");
  url.addQueryItem("start",QString::number(pageIndex()));

  return url;
}

bool Item::doProcessSearchResult(const QByteArray &response, int &newPagesCount)
{
  bool respError = false, wallpaperFound = false;
  QXmlStreamReader xmlResp(response);

  newPagesCount = 1000000;
  while ((!xmlResp.atEnd()) && (!respError))
    switch (xmlResp.readNext())
    {
      case QXmlStreamReader::StartElement:
        if (!xmlResp.name().toString().compare("wallpaper",Qt::CaseInsensitive))
          wallpaperFound = true;
        else if (!xmlResp.name().toString().compare("id",Qt::CaseInsensitive) &&
                 _photoId.isEmpty())
        {
          xmlResp.readNext();
          _photoId = xmlResp.text().toString();
        }
        else if (!xmlResp.name().toString().compare("name",Qt::CaseInsensitive) &&
                 photoTitle.isEmpty())
        {
          xmlResp.readNext();
          photoTitle = xmlResp.text().toString();
        }
        else if (!xmlResp.name().toString().compare("webpage",Qt::CaseInsensitive) &&
                 _sourceUrl.isEmpty())
        {
          xmlResp.readNext();
          _sourceUrl = xmlResp.text().toString();
        }
        break;

      default:
        break;
    }

  if (!wallpaperFound)
    newPagesCount = -1;

  respError |= !wallpaperFound;

  return (!respError && !_photoId.isEmpty() && !_lastPhotoIds.contains(_photoId));
}

QUrl Item::prepareSizeRequest(HttpEngine::Operation & /* op */)
{
  QUrl url("http://www.vladstudio.com/api/wally/getwallpaper.php");

  url.addQueryItem("id",_photoId);

  return url;
}

QSize Item::doProcessSizeRequestResult(const QByteArray &response)
{
  QList< QByteArray > data = response.split('\n');
  QListIterator< QByteArray > value(data);

  while (value.hasNext())
  {
    QByteArray v = value.next().trimmed();

    if (v.contains("url="))
      _photoUrl = v.split('=').at(1).trimmed();
  }

  return QApplication::desktop()->screenGeometry().size();
}

QUrl Item::prepareDownload(HttpEngine::Operation & /* op */)
{
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
  info.description = photoDescription;
  info.owner = photoOwner;
  info.sourceUrl = _sourceUrl;
  info.searchString = categoryToString(_category);

  return info;
}

void Item::doLoadSettings(QSettings &settings)
{
  QByteArray data;
  QBuffer buf(&data);
  QDataStream in(&buf);

  setCategory(static_cast<Category> (settings.value(CATEGORY,All).toInt()));
  setViewOrder(static_cast<ViewOrder> (settings.value(VIEW_ORDER,ById).toInt()));
  setOrderDirection(static_cast<OrderDirection> (settings.value(ORDER_DIRECTION,Ascending).toInt()));

  data = qUncompress(settings.value(LAST_PHOTO_IDS,QByteArray()).toByteArray());

  buf.open(QIODevice::ReadOnly);
  in >> _lastPhotoIds;
  buf.close();
}

void Item::doSaveSettings(QSettings &settings) const
{
  settings.setValue(CATEGORY,static_cast<int> (category()));
  settings.setValue(VIEW_ORDER,static_cast<int> (viewOrder()));
  settings.setValue(ORDER_DIRECTION,static_cast<int> (orderDirection()));
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

bool Item::equalTo(Engine::Item *item) const
{
  Item *vladstudioItem = qobject_cast<Item *> (item);

  return vladstudioItem && (category() == vladstudioItem->category()) &&
                           (viewOrder() == vladstudioItem->viewOrder()) &&
                           (orderDirection() == vladstudioItem->orderDirection());
}

DialogWidget::DialogWidget(XtEngine::Core *core, QWidget *parent) : XtEngine::DialogWidget(core,parent)
{
  QFormLayout *formLayout = new QFormLayout(this);

  cbCategory = new QComboBox(this);
  cbCategory->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbCategory->addItem(tr("All"),All);
  cbCategory->addItem(tr("Abstract art"),AbstractArt);
  cbCategory->addItem(tr("Creatures"),Creatures);
  cbCategory->addItem(tr("Illustrations"),Illustrations);
  cbCategory->addItem(tr("Photos"),Photos);
  cbCategory->setCurrentIndex(0);

  cbViewOrder = new QComboBox(this);
  cbViewOrder->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbViewOrder->addItem(tr("By ID"),ById);
  cbViewOrder->addItem(tr("By view count"),ByView);
  cbViewOrder->setCurrentIndex(0);

  cbOrderDirection = new QComboBox(this);
  cbOrderDirection->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbOrderDirection->addItem(tr("Ascending"),Ascending);
  cbOrderDirection->addItem(tr("Descending"),Descending);
  cbOrderDirection->setCurrentIndex(0);

  formLayout->addRow(new QLabel(tr("Category:"),this),cbCategory);
  formLayout->addRow(new QLabel(tr("Order:"),this),cbViewOrder);
  formLayout->addRow(new QLabel(tr("Direction:"),this),cbOrderDirection);
}

XtEngine::Item *DialogWidget::returnItem()
{
  Item *item = qobject_cast<Item *> (core()->newItem());

  item->setCategory(static_cast<Category>
                     (cbCategory->itemData(cbCategory->currentIndex()).toInt()));
  item->setViewOrder(static_cast<ViewOrder>
                      (cbViewOrder->itemData(cbViewOrder->currentIndex()).toInt()));
  item->setOrderDirection(static_cast<OrderDirection>
                           (cbOrderDirection->itemData(cbOrderDirection->currentIndex()).toInt()));
  return item;
}

void DialogWidget::setupFromItem(XtEngine::Item *item)
{
  if (item)
  {
    Item *vladstudioItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit Vladstudio item"));

    cbCategory->setCurrentIndex(cbCategory->findData(vladstudioItem->category()));
    cbViewOrder->setCurrentIndex(cbViewOrder->findData(vladstudioItem->viewOrder()));
    cbOrderDirection->setCurrentIndex(cbOrderDirection->findData(vladstudioItem->orderDirection()));
  }
  else
    setWindowTitle(tr("Add Vladstudio item"));
}
