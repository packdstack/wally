/*
TRANSLATOR Bing::Item
*/
/*
TRANSLATOR Bing::Core
*/
/*
TRANSLATOR Bing::DialogWidget
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
#include "bing.h"

using namespace Bing;

Item::Item() : HttpEngine::Item()
{
  _tagsCondition = And;
  _adultFilter = Strict;
}

Item::Item(Item *item) : HttpEngine::Item(item)
{
  _tags = item->_tags;
  _tagsCondition = item->_tagsCondition;
  _adultFilter = item->_adultFilter;
}

QVariant Item::data() const
{
  QString str;

  if (_tags.size())
  {
    str = QString("<font size=\"+1\"><b>") + tr("Tags:") + "</b> " +
          _tags.join((_tagsCondition == And)? " <i>" +
          tr("and") + "</i> " : " <i>" + tr("or") + "</i> ") + "</font>" +
          "<br>&nbsp;&nbsp;<i>" + tr("Adult filter:") + " ";

    switch (_adultFilter)
    {
      case Off:
        str += tr("Off");
        break;

      case Moderate:
        str += tr("Moderate");
        break;

      case Strict:
      default:
        str += tr("Strict");
        break;
    }

    str += "</i>";
  }

  return str;
}

int Item::doCalculateNextIndex(bool randomMode, int currentIndex, int pages) const
{
  return (pages)? (currentIndex + ((randomMode)? (qrand() % 5) + 1 : 1) % pages) : 0;
}

QUrl Item::doPrepareInit(HttpEngine::Operation & /* op */)
{
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
  QUrl url("http://api.bing.net/xml.aspx");

  url.addQueryItem("AppId",BING_API_KEY);

  if (_tags.size())
    switch (_tagsCondition)
    {
      case And:
        url.addQueryItem("Query",_tags.join("+"));
        break;

      case Or:
      default:
        url.addQueryItem("Query",_tags.join(" "));
        break;
    }
  else
    return QUrl();

  url.addQueryItem("Sources","Image");
  url.addQueryItem("Version","2.1");

  switch (_adultFilter)
  {
    case Off:
      url.addQueryItem("Adult","Off");
      break;

    case Moderate:
      url.addQueryItem("Adult","Moderate");
      break;

    case Strict:
    default:
      url.addQueryItem("Adult","Strict");
      break;
  }

  url.addQueryItem("Image.Count","1");
  url.addQueryItem("Image.Offset",QString::number(pageIndex()));
  url.addQueryItem("Image.Filters","Size:Large");
  url.addQueryItem("Options","DisableLocationDetection");

  return url;
}

bool Item::doProcessSearchResult(const QByteArray &response, int &newPagesCount)
{
  bool respError = false, thumbnail = false;
  QXmlStreamReader xmlResp(response);

  while ((!xmlResp.atEnd()) && (!respError))
    switch (xmlResp.readNext())
    {
      case QXmlStreamReader::StartElement:
        if (!xmlResp.prefix().toString().compare("mms",Qt::CaseInsensitive) &&
            !xmlResp.name().toString().compare("Thumbnail",Qt::CaseInsensitive))
          thumbnail = true;
        else if (!xmlResp.prefix().toString().compare("mms",Qt::CaseInsensitive) &&
                 !xmlResp.name().toString().compare("Total",Qt::CaseInsensitive))
        {
          xmlResp.readNext();
          newPagesCount = xmlResp.text().toString().toInt();
        }
        else if (!xmlResp.prefix().toString().compare("mms",Qt::CaseInsensitive) &&
                 !xmlResp.name().toString().compare("Title",Qt::CaseInsensitive))
        {
          xmlResp.readNext();
          photoTitle = xmlResp.text().toString();
        }
        else if (!xmlResp.prefix().toString().compare("mms",Qt::CaseInsensitive) &&
                 !xmlResp.name().toString().compare("MediaUrl",Qt::CaseInsensitive))
        {
          xmlResp.readNext();
          _photoUrl = xmlResp.text().toString();
        }
        else if (!xmlResp.prefix().toString().compare("mms",Qt::CaseInsensitive) &&
                 !xmlResp.name().toString().compare("Height",Qt::CaseInsensitive) &&
                 !thumbnail)
        {
          xmlResp.readNext();
          photoSize.setHeight(xmlResp.text().toString().toInt());
        }
        else if (!xmlResp.prefix().toString().compare("mms",Qt::CaseInsensitive) &&
                 !xmlResp.name().toString().compare("Width",Qt::CaseInsensitive) &&
                 !thumbnail)
        {
          xmlResp.readNext();
          photoSize.setWidth(xmlResp.text().toString().toInt());
        }
        else if (!xmlResp.prefix().toString().compare("mms",Qt::CaseInsensitive) &&
                 !xmlResp.name().toString().compare("Url",Qt::CaseInsensitive) &&
                 !thumbnail)
        {
          xmlResp.readNext();
          _sourceUrl = xmlResp.text().toString();
        }
        break;

      case QXmlStreamReader::EndElement:
        if (!xmlResp.prefix().toString().compare("mms",Qt::CaseInsensitive) &&
            !xmlResp.name().toString().compare("Thumbnail",Qt::CaseInsensitive))
          thumbnail = false;
        break;

      default:
        break;
    }

  return (!respError && _photoUrl.isValid() && !_lastPhotoUrls.contains(_photoUrl));
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
  return _photoUrl;
}

QUrl Item::prepareInfoCollect(HttpEngine::Operation & /* op */)
{
  _lastPhotoUrls.insert(_photoUrl);
  return QUrl("/");
}

PhotoInfo Item::doProcessInfoCollectResult(const QByteArray & /* response */)
{
  PhotoInfo info;

  info.title = photoTitle;
  info.description = photoDescription;
  info.owner = photoOwner;
  info.sourceUrl = _sourceUrl;
  info.searchString = _tags.join(" ");

  return info;
}

void Item::doLoadSettings(QSettings &settings)
{
  QByteArray data;
  QBuffer buf(&data);
  QDataStream in(&buf);

  _tags = settings.value(TAGS,QString()).toString().split(" ");
  _tagsCondition = static_cast<TagsCondition> (settings.value(TAGS_CONDITION,And).toInt());
  setAdultFilter(static_cast<AdultFilter> (settings.value(ADULT_FILTER,Strict).toInt()));

  data = qUncompress(settings.value(LAST_PHOTO_URLS,QByteArray()).toByteArray());

  buf.open(QIODevice::ReadOnly);
  in >> _lastPhotoUrls;
  buf.close();
}

void Item::doSaveSettings(QSettings &settings) const
{
  settings.setValue(TAGS,_tags.join(" "));
  settings.setValue(TAGS_CONDITION,_tagsCondition);
  settings.setValue(ADULT_FILTER,static_cast<int> (_adultFilter));
}

void Item::doSaveState(QSettings &settings) const
{
  QByteArray data;
  QBuffer buf(&data);
  QDataStream out(&buf);

  buf.open(QIODevice::WriteOnly);
  out << _lastPhotoUrls;
  buf.close();

  settings.setValue(LAST_PHOTO_URLS,qCompress(data));
}

bool Item::equalTo(Engine::Item *item) const
{
  Item *bingItem = qobject_cast<Item *> (item);

  return bingItem && (tags().join(";") == bingItem->tags().join(";")) &&
                     (tagsCondition() == bingItem->tagsCondition()) &&
                     (adultFilter() == bingItem->adultFilter());
}

DialogWidget::DialogWidget(XtEngine::Core *core, QWidget *parent) : XtEngine::DialogWidget(core,parent)
{
  QVBoxLayout *mainLayout = new QVBoxLayout;
  QHBoxLayout *topLayout = new QHBoxLayout;
  QHBoxLayout *bottomLayout = new QHBoxLayout;

  leSearchFor = new QLineEdit(this);
  leSearchFor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  cbTagsCondition = new QComboBox(this);
  cbTagsCondition->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbTagsCondition->addItem(tr("All of these words"),And);
  cbTagsCondition->addItem(tr("Any of these words"),Or);
  cbTagsCondition->setCurrentIndex(0);

  topLayout->addWidget(new QLabel(tr("Search for:"),this));
  topLayout->addWidget(leSearchFor);
  topLayout->addWidget(cbTagsCondition);

  cbAdultFilter = new QComboBox(this);
  cbAdultFilter->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbAdultFilter->addItem(tr("Strict"),Strict);
  cbAdultFilter->addItem(tr("Moderate"),Moderate);
  cbAdultFilter->addItem(tr("Off"),Off);
  cbAdultFilter->setCurrentIndex(0);

  bottomLayout->addWidget(new QLabel(tr("Adult filter:"),this));
  bottomLayout->addWidget(cbAdultFilter);
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
  item->setTagsCondition(static_cast<TagsCondition>
                          (cbTagsCondition->itemData(cbTagsCondition->currentIndex()).toInt()));
  item->setAdultFilter(static_cast<AdultFilter>
                        (cbAdultFilter->itemData(cbAdultFilter->currentIndex()).toInt()));

  if (item->adultFilter() == Off)
    QMessageBox::warning(this,tr("Bing item"),
                         tr("Unfiltered content can show offending or sexual explicit photos"));

  return item;
}

void DialogWidget::setupFromItem(XtEngine::Item *item)
{
  if (item)
  {
    Item *bingItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit Bing item"));

    leSearchFor->setText(bingItem->tags().join(" "));
    cbTagsCondition->setCurrentIndex(cbTagsCondition->findData(bingItem->tagsCondition()));
    cbAdultFilter->setCurrentIndex(cbAdultFilter->findData(bingItem->adultFilter()));
  }
  else
    setWindowTitle(tr("Add Bing item"));
}
