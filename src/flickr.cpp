/*
TRANSLATOR Flickr::Item
*/
/*
TRANSLATOR Flickr::Core
*/
/*
TRANSLATOR Flickr::DialogWidget
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
#include "flickr.h"

using namespace Flickr;

Item::Item() : HttpEngine::Item()
{
  _tagsCondition = And;
  _searchOrder = DatePostedAsc;
  _requestedSize = Largest;
}

Item::Item(Item *item) : HttpEngine::Item(item)
{
  _text = item->_text;
  _tags = item->_tags;
  _tagsCondition = item->_tagsCondition;
  _searchOrder = item->_searchOrder;
  _requestedSize = item->_requestedSize;
}

QVariant Item::data() const
{
  QString str;

  if (!_text.isEmpty())
    str = QString("<font size=\"+1\"><b>") + tr("Text:") + "</b> " + _text + "</font>";
  else if (_tags.size())
    str = QString("<font size=\"+1\"><b>") + tr("Tags:") + "</b> " +
          _tags.join((_tagsCondition == And)? " <i>" + tr("and") + "</i> " :
                                              " <i>" + tr("or") + "</i> ") + "</font>";

  if (!str.isEmpty())
  {
    str += "<br>&nbsp;&nbsp;<i>" + tr("Size:") + " ";
    switch (_requestedSize)
    {
      case Largest: str += tr("Largest"); break;
      case Original: str += tr("Original"); break;
      case Large: str += tr("Large"); break;
      case Medium: str += tr("Medium"); break;
      case Small: str += tr("Small"); break;
      case Thumbnail: str += tr("Thumbnail"); break;
      case Square: str += tr("Square"); break;
      case Smallest: str += tr("Smallest"); break;
    }

    str += ",&nbsp;&nbsp;" + tr("Order:") + " ";
    switch (_searchOrder)
    {
      case InterestingnessDesc: str += tr("Interestingness desc"); break;
      case Relevance: str += tr("Relevance"); break;
      case DatePostedDesc: str += tr("Date posted desc"); break;
      case DateTakenDesc: str += tr("Date taken desc"); break;
      case InterestingnessAsc: str += tr("Interestingness asc"); break;
      case DatePostedAsc: str += tr("Date posted asc"); break;
      case DateTakenAsc: str += tr("Date taken asc"); break;
    }

    str += "</i>";
  }

  return str;
}

int Item::doCalculateNextIndex(bool randomMode, int currentIndex, int pages) const
{
  return (pages)? (currentIndex + ((randomMode)? (qrand() % 5) + 1 : 1)) % pages : 1;
}

QUrl Item::doPrepareInit(HttpEngine::Operation & /* op */)
{
  _photoId.clear();
  _photoUrl.clear();

  return QUrl();
}

QUrl Item::doPrepareSearch(HttpEngine::Operation & /* op */)
{
  QUrl url("http://api.flickr.com/services/rest");

  url.addQueryItem("method","flickr.photos.search");
  url.addQueryItem("api_key",FLICKR_API_KEY);

  if (!_tags.isEmpty())
  {
    url.addQueryItem("tags",_tags.join(","));
    url.addQueryItem("tag_mode",(_tagsCondition == And)? "all" : "any");
  }
  else if (!_text.isEmpty())
  {
    QString newText = _text;

    newText.replace(" ","+");
    url.addQueryItem("text",newText);
  }
  else
    return QUrl();

  url.addQueryItem("media","photos");
  url.addQueryItem("sort",searchOrderStrings[_searchOrder]);
  url.addQueryItem("per_page","1");

  url.addQueryItem("page",QString::number(pageIndex()));

  return url;
}

bool Item::doProcessSearchResult(const QByteArray &response, int &newPagesCount)
{
  bool respError = false;
  QXmlStreamReader xmlResp(response);

  while (!xmlResp.atEnd() && !respError)
    if (xmlResp.readNext() == QXmlStreamReader::StartElement)
    {
      if (!xmlResp.name().toString().compare("rsp",Qt::CaseInsensitive))
        respError = xmlResp.attributes().value("stat").toString().compare("ok",Qt::CaseInsensitive);
      else if (!xmlResp.name().toString().compare("photos",Qt::CaseInsensitive))
        newPagesCount = qMin(xmlResp.attributes().value("total").toString().toInt(),
                             FLICKR_PAGES_HARD_LIMIT);
      else if (!xmlResp.name().toString().compare("photo",Qt::CaseInsensitive) &&
               (xmlResp.attributes().value("ispublic").toString() == "1"))
        _photoId = xmlResp.attributes().value("id").toString();
    }

  return (!respError && !_photoId.isEmpty() && !_lastPhotoIds.contains(_photoId));
}

QUrl Item::prepareSizeRequest(HttpEngine::Operation & /* op */)
{
  QUrl url("http://api.flickr.com/services/rest");

  url.addQueryItem("method","flickr.photos.getSizes");
  url.addQueryItem("api_key",FLICKR_API_KEY);
  url.addQueryItem("photo_id",_photoId);

  return url;
}

QSize Item::doProcessSizeRequestResult(const QByteArray &response)
{
  bool respError = false, found = false;
  QSize size(0,0);

  QXmlStreamReader xmlResp(response);

  while ((!xmlResp.atEnd()) && (!respError) && (!found))
    if (xmlResp.readNext() == QXmlStreamReader::StartElement)
    {
      if (!xmlResp.name().toString().compare("rsp",Qt::CaseInsensitive))
        respError = xmlResp.attributes().value("stat").toString().compare("ok",Qt::CaseInsensitive);
      else if (!xmlResp.name().toString().compare("size",Qt::CaseInsensitive))
      {
        switch (_requestedSize)
        {
          case Large:
            found = !xmlResp.attributes().value("label").toString().compare("Large",Qt::CaseInsensitive);
            _photoUrl.setUrl(xmlResp.attributes().value("source").toString());
            size.setHeight(xmlResp.attributes().value("height").toString().toInt());
            size.setWidth(xmlResp.attributes().value("width").toString().toInt());
            break;

          case Largest:
            if (_photoUrl.isEmpty() ||
                (xmlResp.attributes().value("width").toString().toInt() *
                 xmlResp.attributes().value("height").toString().toInt() >= size.height() * size.width()))
            {
              _photoUrl.setUrl(xmlResp.attributes().value("source").toString());
              size.setHeight(xmlResp.attributes().value("height").toString().toInt());
              size.setWidth(xmlResp.attributes().value("width").toString().toInt());
            }
            break;

          case Medium:
            found = !xmlResp.attributes().value("label").toString().compare("Medium",Qt::CaseInsensitive);
            _photoUrl.setUrl(xmlResp.attributes().value("source").toString());
            size.setHeight(xmlResp.attributes().value("height").toString().toInt());
            size.setWidth(xmlResp.attributes().value("width").toString().toInt());
            break;

          case Original:
            found = !xmlResp.attributes().value("label").toString().compare("Original",Qt::CaseInsensitive);
            _photoUrl.setUrl(xmlResp.attributes().value("source").toString());
            size.setHeight(xmlResp.attributes().value("height").toString().toInt());
            size.setWidth(xmlResp.attributes().value("width").toString().toInt());
            break;

          case Small:
            found = !xmlResp.attributes().value("label").toString().compare("Small",Qt::CaseInsensitive);
            _photoUrl.setUrl(xmlResp.attributes().value("source").toString());
            size.setHeight(xmlResp.attributes().value("height").toString().toInt());
            size.setWidth(xmlResp.attributes().value("width").toString().toInt());
            break;

          case Smallest:
            if (_photoUrl.isEmpty() ||
                (xmlResp.attributes().value("width").toString().toInt() *
                 xmlResp.attributes().value("height").toString().toInt() <= size.height() * size.width()))
            {
              _photoUrl.setUrl(xmlResp.attributes().value("source").toString());
              size.setHeight(xmlResp.attributes().value("height").toString().toInt());
              size.setWidth(xmlResp.attributes().value("width").toString().toInt());
            }
            break;

          case Square:
            found = !xmlResp.attributes().value("label").toString().compare("Square",Qt::CaseInsensitive);
            _photoUrl.setUrl(xmlResp.attributes().value("source").toString());
            size.setHeight(xmlResp.attributes().value("height").toString().toInt());
            size.setWidth(xmlResp.attributes().value("width").toString().toInt());
            break;

          case Thumbnail:
            found = !xmlResp.attributes().value("label").toString().compare("Thumbnail",Qt::CaseInsensitive);
            _photoUrl.setUrl(xmlResp.attributes().value("source").toString());
            size.setHeight(xmlResp.attributes().value("height").toString().toInt());
            size.setWidth(xmlResp.attributes().value("width").toString().toInt());
            break;
        }
      }
    }

  if (found || (_requestedSize == Largest) || (_requestedSize == Smallest))
    return size;

  return QSize(-1,-1);
}

QUrl Item::prepareDownload(HttpEngine::Operation & /* op */)
{
  return _photoUrl;
}

QUrl Item::prepareInfoCollect(HttpEngine::Operation & /* op */)
{
  QUrl url("http://api.flickr.com/services/rest");

  _lastPhotoIds.insert(_photoId);

  url.addQueryItem("method","flickr.photos.getInfo");
  url.addQueryItem("api_key",FLICKR_API_KEY);
  url.addQueryItem("photo_id",_photoId);

  return url;
}

PhotoInfo Item::doProcessInfoCollectResult(const QByteArray &response)
{
  PhotoInfo info;
  bool respError = false;

  QXmlStreamReader xmlResp(response);

  while ((!xmlResp.atEnd()) && (!respError))
    if (xmlResp.readNext() == QXmlStreamReader::StartElement)
    {
      if (!xmlResp.name().toString().compare("rsp",Qt::CaseInsensitive))
        respError = xmlResp.attributes().value("stat").toString().compare("ok",Qt::CaseInsensitive);
      else if (!xmlResp.name().toString().compare("title",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        info.title = xmlResp.text().toString();
      }
      else if (!xmlResp.name().toString().compare("description",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        info.description = xmlResp.text().toString();
      }
      else if (!xmlResp.name().toString().compare("owner",Qt::CaseInsensitive))
      {
        info.owner = xmlResp.attributes().value("realname").toString();
        if (info.owner.isEmpty())
          info.owner = xmlResp.attributes().value("username").toString();
        info.location = xmlResp.attributes().value("location").toString();
      }
      else if (!xmlResp.name().toString().compare("url",Qt::CaseInsensitive) &&
               !xmlResp.attributes().value("type").toString().compare("photopage",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        info.sourceUrl = xmlResp.text().toString();
      }
    }

  if (!_tags.isEmpty())
    info.searchString = _tags.join(" ");
  else if (!_text.isEmpty())
    info.searchString = _text;

  return info;
}

void Item::doLoadSettings(QSettings &settings)
{
  QByteArray data;
  QBuffer buf(&data);
  QDataStream in(&buf);

  if (!settings.value(SEARCH_TAGS,QString()).toString().isEmpty())
  {
    _tags = settings.value(SEARCH_TAGS,QString()).toString().split(" ");
    _tagsCondition = static_cast<TagsCondition> (settings.value(TAGS_CONDITION,And).toInt());
  }
  else if (!settings.value(SEARCH_TEXT,QString()).toString().isEmpty())
    _text = settings.value(SEARCH_TEXT,QString()).toString();

  data = qUncompress(settings.value(LAST_PHOTO_IDS,QByteArray()).toByteArray());

  buf.open(QIODevice::ReadOnly);
  in >> _lastPhotoIds;
  buf.close();

  setSearchOrder(static_cast<SearchOrder> (settings.value(SEARCH_ORDER,DatePostedAsc).toInt()));
  setRequestedSize(static_cast<RequestedSize> (settings.value(REQUESTED_SIZE,Largest).toInt()));
}

void Item::doSaveSettings(QSettings &settings) const
{
  if (_tags.size())
    settings.setValue(SEARCH_TAGS,_tags.join(" "));
  else if (!_text.isEmpty())
    settings.setValue(SEARCH_TEXT,_text);
  settings.setValue(TAGS_CONDITION,_tagsCondition);
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

bool Item::equalTo(Engine::Item *item) const
{
  Item *flickrItem = qobject_cast<Item *> (item);

  return flickrItem && (requestedSize() == flickrItem->requestedSize()) &&
                       (tagsCondition() == flickrItem->tagsCondition()) &&
                       (searchOrder() == flickrItem->searchOrder()) &&
                       (text() == flickrItem->text()) &&
                       (tags() == flickrItem->tags());
}

DialogWidget::DialogWidget(XtEngine::Core *core, QWidget *parent) : XtEngine::DialogWidget(core,parent)
{
  QVBoxLayout *mainLayout = new QVBoxLayout;
  QHBoxLayout *topLayout = new QHBoxLayout;
  QHBoxLayout *radioLayout = new QHBoxLayout;
  QHBoxLayout *bottomLayout = new QHBoxLayout;

  leSearchFor = new QLineEdit(this);
  leSearchFor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  cbTagsCondition = new QComboBox(this);
  cbTagsCondition->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbTagsCondition->addItem(tr("All of these words"),And);
  cbTagsCondition->addItem(tr("Any of these words"),Or);
  cbTagsCondition->setCurrentIndex(0);
  cbTagsCondition->setEnabled(false);

  topLayout->addWidget(new QLabel(tr("Search for:"),this));
  topLayout->addWidget(leSearchFor);
  topLayout->addWidget(cbTagsCondition);

  rbText = new QRadioButton(tr("Full text"),this);
  rbTags = new QRadioButton(tr("Tags only"),this);
  radioLayout->addWidget(rbText);
  radioLayout->addWidget(rbTags);
  radioLayout->addStretch();
  rbText->setChecked(true);

  cbRequestedSize = new QComboBox(this);
  cbRequestedSize->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbRequestedSize->addItem(tr("Largest"),Largest);
  cbRequestedSize->addItem(tr("Original"),Original);
  cbRequestedSize->addItem(tr("Large"),Large);
  cbRequestedSize->addItem(tr("Medium"),Medium);
  cbRequestedSize->addItem(tr("Small"),Small);
  cbRequestedSize->addItem(tr("Thumbnail"),Thumbnail);
  cbRequestedSize->addItem(tr("Square"),Square);
  cbRequestedSize->addItem(tr("Smallest"),Smallest);
  cbRequestedSize->setCurrentIndex(0);

  cbSearchOrder = new QComboBox(this);
  cbSearchOrder->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbSearchOrder->addItem(tr("Interestingness desc"),InterestingnessDesc);
  cbSearchOrder->addItem(tr("Relevance"),Relevance);
  cbSearchOrder->addItem(tr("Date posted desc"),DatePostedDesc);
  cbSearchOrder->addItem(tr("Date taken desc"),DateTakenDesc);
  cbSearchOrder->addItem(tr("Interestingness asc"),InterestingnessAsc);
  cbSearchOrder->addItem(tr("Date posted asc"),DatePostedAsc);
  cbSearchOrder->addItem(tr("Date taken asc"),DateTakenAsc);
  cbSearchOrder->setCurrentIndex(0);

  bottomLayout->addWidget(new QLabel(tr("Size:"),this));
  bottomLayout->addWidget(cbRequestedSize);
  bottomLayout->addWidget(new QLabel(tr("Order:"),this));
  bottomLayout->addWidget(cbSearchOrder);
  bottomLayout->addStretch();

  connect(rbTags,SIGNAL(toggled(bool)),cbTagsCondition,SLOT(setEnabled(bool)));

  mainLayout->addLayout(topLayout);
  mainLayout->addLayout(radioLayout);
  mainLayout->addLayout(bottomLayout);

  setLayout(mainLayout);

  leSearchFor->setFocus();
}

XtEngine::Item *DialogWidget::returnItem()
{
  Item *item = qobject_cast<Item *> (core()->newItem());

  if (rbTags->isChecked())
  {
    item->setTags(leSearchFor->text().split(" "));
    item->setTagsCondition(static_cast<TagsCondition>
                             (cbTagsCondition->itemData(cbTagsCondition->currentIndex()).toInt()));
  }
  else
    item->setText(leSearchFor->text());

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
    Item *flickrItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit Flickr item"));

    if (!flickrItem->text().isEmpty())
    {
      leSearchFor->setText(flickrItem->text());
      rbText->setChecked(true);
    }
    else
    {
      leSearchFor->setText(flickrItem->tags().join(" "));
      rbTags->setChecked(true);
    }

    cbTagsCondition->setCurrentIndex(cbTagsCondition->findData(flickrItem->tagsCondition()));
    cbSearchOrder->setCurrentIndex(cbSearchOrder->findData(flickrItem->searchOrder()));
    cbRequestedSize->setCurrentIndex(cbRequestedSize->findData(flickrItem->requestedSize()));
  }
  else
    setWindowTitle(tr("Add Flickr item"));
}
