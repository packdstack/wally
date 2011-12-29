/*
TRANSLATOR Pikeo::Item
*/
/*
TRANSLATOR Pikeo::Core
*/
/*
TRANSLATOR Pikeo::DialogWidget
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
#include "pikeo.h"

using namespace Pikeo;

Item::Item() : HttpEngine::Item()
{
  _searchOrder = Default;
  _ascending = false;
}

Item::Item(Item *item) : HttpEngine::Item(item)
{
  _tags = item->_tags;
  _searchOrder = item->_searchOrder;
  _ascending = item->_ascending;
}

QVariant Item::data() const
{
  QString str;

  if (_tags.size())
  {
    str = QString("<font size=\"+1\"><b>") + tr("Tags:") + "</b> " +
          _tags.join(" ") + "</font><br>&nbsp;&nbsp;<i>" + tr("Order:") + " ";

    switch (_searchOrder)
    {
      case Default: str += tr("Default"); break;
      case MostViewed: str += tr("Most viewed"); break;
      case UploadDate: str += tr("Upload date"); break;
      case DateTaken: str += tr("Date taken"); break;
      case GroupAddDate: str += tr("Group add date"); break;
      case CommentDate: str += tr("Comment date"); break;
    }

    str += " (" + ((_ascending)? tr("ascending") : tr("descending")) + ")</i>";
  }

  return str;
}

int Item::doCalculateNextIndex(bool randomMode, int currentIndex, int /* pages */) const
{
  return currentIndex + ((randomMode)? (qrand() % 5) + 1 : 1);
}

QUrl Item::doPrepareInit(HttpEngine::Operation & /* op */)
{
  _photoId.clear();
  urlPrefix.clear();
  urlFile.clear();
  photoTitle.clear();
  photoDescription.clear();
  photoOwner.clear();
  photoSize = QSize(-1,-1);

  return QUrl();
}

QUrl Item::doPrepareSearch(HttpEngine::Operation &op)
{
  QUrl url("http://api.pikeo.com/services/pikeo/v2/rest");
  QCryptographicHash hash(QCryptographicHash::Sha1);
  QByteArray nonce;
  QByteArray nonceTimestampStr = QDateTime::currentDateTime().toString(Qt::ISODate).toAscii() + "Z";
  int nonceLength = (qrand() % 10) + 10;

  op = HttpEngine::Post;

  while (--nonceLength)
    nonce += qrand() % 10 + 0x30;

  hash.addData(nonce.toBase64() + nonceTimestampStr + QByteArray(PIKEO_SECRET_KEY));

  url.addEncodedQueryItem("api_sig",QUrl::toPercentEncoding(hash.result().toBase64()));
  url.addEncodedQueryItem("nonce",QUrl::toPercentEncoding(nonce.toBase64()));
  url.addEncodedQueryItem("timestamp",QUrl::toPercentEncoding(nonceTimestampStr));
  url.addQueryItem("api_key",PIKEO_API_KEY);
  url.addQueryItem("method","pikeo.photos.search");
  url.addQueryItem("num_items","1");
  url.addQueryItem("only_public","true");
  url.addQueryItem("offset",QString::number(pageIndex()));
  if (_tags.size())
    url.addQueryItem("text",QUrl::toPercentEncoding(_tags.join(" ")));
  url.addQueryItem("order_type",QString::number(_searchOrder));
  url.addQueryItem("order_asc",QString((_ascending)? "true" : "false"));

  return url;
}

bool Item::doProcessSearchResult(const QByteArray &response, int & /* newPagesCount */)
{
  QXmlStreamReader xmlResp(response);

  while (!xmlResp.atEnd())
    if (xmlResp.readNext() == QXmlStreamReader::StartElement)
    {
      if (!xmlResp.name().toString().compare("id",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        _photoId = xmlResp.text().toString();
      }
      else if (!xmlResp.name().toString().compare("url_prefix",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        urlPrefix = xmlResp.text().toString() + "upload/";
      }
      else if (!xmlResp.name().toString().compare("original",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        urlFile = xmlResp.text().toString();
      }
      else if (!xmlResp.name().toString().compare("title",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        photoTitle = xmlResp.text().toString();
      }
      else if (!xmlResp.name().toString().compare("owner_username",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        photoOwner = xmlResp.text().toString();
      }
      else if (!xmlResp.name().toString().compare("description",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        photoDescription = xmlResp.text().toString();
      }
      else if (!xmlResp.name().toString().compare("width",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        photoSize.setWidth(xmlResp.text().toString().toInt());
      }
      else if (!xmlResp.name().toString().compare("height",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        photoSize.setHeight(xmlResp.text().toString().toInt());
      }
    }

  if (_photoId.isEmpty())
    resetPages();

  return !_photoId.isEmpty() && !_lastPhotoIds.contains(_photoId) &&
         !urlPrefix.isEmpty() && !urlFile.isEmpty();
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
  return QUrl(urlPrefix + urlFile);
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
  info.searchString = _tags.join(" ");

  return info;
}

void Item::doLoadSettings(QSettings &settings)
{
  QByteArray data;
  QBuffer buf(&data);
  QDataStream in(&buf);

  _tags = settings.value(TAGS,QString()).toString().split(" ");
  _searchOrder = static_cast<SearchOrder> (settings.value(SEARCH_ORDER,Default).toInt());
  _ascending = settings.value(ASCENDING,false).toBool();

  data = qUncompress(settings.value(LAST_PHOTO_IDS,QByteArray()).toByteArray());

  buf.open(QIODevice::ReadOnly);
  in >> _lastPhotoIds;
  buf.close();
}

void Item::doSaveSettings(QSettings &settings) const
{
  settings.setValue(TAGS,_tags.join(" "));
  settings.setValue(SEARCH_ORDER,_searchOrder);
  settings.setValue(ASCENDING,_ascending);
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
  Item *pikeoItem = qobject_cast<Item *> (item);

  return pikeoItem && (tags().join(";") == pikeoItem->tags().join(";")) &&
                       (searchOrder() == pikeoItem->searchOrder()) &&
                       (isAscending() == pikeoItem->isAscending());
}

DialogWidget::DialogWidget(XtEngine::Core *core, QWidget *parent) : XtEngine::DialogWidget(core,parent)
{
  QVBoxLayout *mainLayout = new QVBoxLayout;
  QHBoxLayout *topLayout = new QHBoxLayout;
  QHBoxLayout *bottomLayout = new QHBoxLayout;

  leSearchFor = new QLineEdit(this);
  leSearchFor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  topLayout->addWidget(new QLabel(tr("Search for:"),this));
  topLayout->addWidget(leSearchFor);

  cbSearchOrder = new QComboBox(this);
  cbSearchOrder->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbSearchOrder->addItem(tr("Default"),Default);
  cbSearchOrder->addItem(tr("Most viewed"),MostViewed);
  cbSearchOrder->addItem(tr("Upload date"),UploadDate);
  cbSearchOrder->addItem(tr("Date taken"),DateTaken);
  cbSearchOrder->addItem(tr("Group add date"),GroupAddDate);
  cbSearchOrder->addItem(tr("Comment date"),CommentDate);
  cbSearchOrder->setCurrentIndex(0);

  cbAscendingOrder = new QCheckBox(this);
  cbAscendingOrder->setText(tr("Ascending"));

  bottomLayout->addWidget(new QLabel(tr("Order:"),this));
  bottomLayout->addWidget(cbSearchOrder);
  bottomLayout->addWidget(cbAscendingOrder);
  bottomLayout->addStretch();

  mainLayout->addLayout(topLayout);
  mainLayout->addLayout(bottomLayout);

  setLayout(mainLayout);

  leSearchFor->setFocus();
}

XtEngine::Item *DialogWidget::returnItem()
{
  Item *item = qobject_cast<Item *> (core()->newItem());

  item->setTags(leSearchFor->text().split(" "));
  item->setSearchOrder(static_cast<SearchOrder>
                         (cbSearchOrder->itemData(cbSearchOrder->currentIndex()).toInt()));
  item->setAscending(cbAscendingOrder->isChecked());

  return item;
}

void DialogWidget::setupFromItem(XtEngine::Item *item)
{
  if (item)
  {
    Item *pikeoItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit Pikeo item"));

    leSearchFor->setText(pikeoItem->tags().join(" "));
    cbSearchOrder->setCurrentIndex(cbSearchOrder->findData(pikeoItem->searchOrder()));
    cbAscendingOrder->setChecked(pikeoItem->isAscending());
  }
  else
    setWindowTitle(tr("Add Pikeo item"));
}
