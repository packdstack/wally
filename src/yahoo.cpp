/*
TRANSLATOR Yahoo::Item
*/
/*
TRANSLATOR Yahoo::Core
*/
/*
TRANSLATOR Yahoo::DialogWidget
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
#include "yahoo.h"

using namespace Yahoo;

Item::Item() : HttpEngine::Item()
{
  _tagsCondition = And;
  _contentFiltered = true;
}

Item::Item(Item *item) : HttpEngine::Item(item)
{
  _tags = item->_tags;
  _tagsCondition = item->_tagsCondition;
  _contentFiltered = item->_contentFiltered;
}

QVariant Item::data() const
{
  QString str;

  if (_tags.size())
  {
    str = QString("<font size=\"+1\"><b>") + tr("Tags:") + "</b> " +
          _tags.join((_tagsCondition == And)? " <i>" +
          tr("and") + "</i> " : " <i>" + tr("or") + "</i> ") + "</font>";

    if (_contentFiltered)
      str += "<br>&nbsp;&nbsp;<i>(" + tr("content filtered") + ")</i>";
  }

  return str;
}

int Item::doCalculateNextIndex(bool randomMode, int currentIndex, int pages) const
{
  return (pages)? (currentIndex + ((randomMode)? (qrand() % 5) + 1 : 1) % pages) : 1;
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
  QUrl url("http://search.yahooapis.com/ImageSearchService/V1/imageSearch");

  url.addQueryItem("appid",YAHOO_API_KEY);

  if (_tags.size())
  {
    url.addQueryItem("query",_tags.join("+"));
    url.addQueryItem("type",(_tagsCondition == And)? "all" : "any");
  }
  else
    return QUrl();

  if (!_contentFiltered)
    url.addQueryItem("adult_ok","1");
  url.addQueryItem("results","1");
  url.addQueryItem("start",QString::number(pageIndex()));

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
        if (!xmlResp.name().toString().compare("Thumbnail",Qt::CaseInsensitive))
          thumbnail = true;
        else if (!xmlResp.name().toString().compare("ResultSet",Qt::CaseInsensitive))
        {
          respError = (xmlResp.attributes().value("totalResultsReturned").toString() != "1");

          newPagesCount = xmlResp.attributes().value("totalResultsAvailable").toString().toInt();
        }
        else if (!xmlResp.name().toString().compare("Title",Qt::CaseInsensitive) &&
                 photoTitle.isEmpty())
        {
          xmlResp.readNext();
          photoTitle = xmlResp.text().toString();
        }
        else if (!xmlResp.name().toString().compare("Summary",Qt::CaseInsensitive) &&
                 photoDescription.isEmpty())
        {
          xmlResp.readNext();
          photoDescription = xmlResp.text().toString();
        }
        else if (!xmlResp.name().toString().compare("Url",Qt::CaseInsensitive) &&
                 _photoUrl.isEmpty())
        {
          xmlResp.readNext();
          _photoUrl = xmlResp.text().toString();
        }
        else if (!xmlResp.name().toString().compare("Publisher",Qt::CaseInsensitive) &&
                 photoOwner.isEmpty())
        {
          xmlResp.readNext();
          photoOwner = xmlResp.text().toString();
        }
        else if (!xmlResp.name().toString().compare("Height",Qt::CaseInsensitive) &&
                 !thumbnail)
        {
          xmlResp.readNext();
          photoSize.setHeight(xmlResp.text().toString().toInt());
        }
        else if (!xmlResp.name().toString().compare("Width",Qt::CaseInsensitive) &&
                 !thumbnail)
        {
          xmlResp.readNext();
          photoSize.setWidth(xmlResp.text().toString().toInt());
        }
        else if (!xmlResp.name().toString().compare("RefererUrl",Qt::CaseInsensitive) &&
                 !thumbnail)
        {
          xmlResp.readNext();
          _sourceUrl = xmlResp.text().toString();
        }
        break;

      case QXmlStreamReader::EndElement:
        if (!xmlResp.name().toString().compare("Thumbnail",Qt::CaseInsensitive))
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
  setFilterContent(settings.value(CONTENT_FILTERED,true).toBool());

  data = qUncompress(settings.value(LAST_PHOTO_URLS,QByteArray()).toByteArray());

  buf.open(QIODevice::ReadOnly);
  in >> _lastPhotoUrls;
  buf.close();
}

void Item::doSaveSettings(QSettings &settings) const
{
  settings.setValue(TAGS,_tags.join(" "));
  settings.setValue(TAGS_CONDITION,_tagsCondition);
  settings.setValue(CONTENT_FILTERED,_contentFiltered);
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
  Item *yahooItem = qobject_cast<Item *> (item);

  return yahooItem && (tags().join(";") == yahooItem->tags().join(";")) &&
                      (tagsCondition() == yahooItem->tagsCondition()) &&
                      (isContentFiltered() == yahooItem->isContentFiltered());
}

DialogWidget::DialogWidget(XtEngine::Core *core, QWidget *parent) : XtEngine::DialogWidget(core,parent)
{
  QVBoxLayout *mainLayout = new QVBoxLayout;
  QHBoxLayout *topLayout = new QHBoxLayout;

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

  cbFilterContent = new QCheckBox(this);
  cbFilterContent->setText(tr("Filter content"));

  mainLayout->addLayout(topLayout);
  mainLayout->addWidget(cbFilterContent);

  setLayout(mainLayout);

  leSearchFor->setFocus();
}

XtEngine::Item *DialogWidget::returnItem()
{
  Item *item = qobject_cast<Item *> (core()->newItem());

  item->setTags(leSearchFor->text().split(" "));
  item->setTagsCondition(static_cast<TagsCondition>
                          (cbTagsCondition->itemData(cbTagsCondition->currentIndex()).toInt()));
  item->setFilterContent(cbFilterContent->isChecked());

  if (!item->isContentFiltered())
    QMessageBox::warning(this,tr("Yahoo! item"),
                         tr("Unfiltered content can show offending or sexual explicit photos"));

  return item;
}

void DialogWidget::setupFromItem(XtEngine::Item *item)
{
  if (item)
  {
    Item *yahooItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit Yahoo! item"));

    leSearchFor->setText(yahooItem->tags().join(" "));
    cbTagsCondition->setCurrentIndex(cbTagsCondition->findData(yahooItem->tagsCondition()));
    cbFilterContent->setChecked(yahooItem->isContentFiltered());
  }
  else
  {
    setWindowTitle(tr("Add Yahoo! item"));
    cbFilterContent->setChecked(true);
  }
}
