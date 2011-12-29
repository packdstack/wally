/*
TRANSLATOR Ipernity::Item
*/
/*
TRANSLATOR Ipernity::Core
*/
/*
TRANSLATOR Ipernity::DialogWidget
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
#include "ipernity.h"

using namespace Ipernity;

Item::Item() : HttpEngine::Item()
{
  _condition = And;
}

Item::Item(Item *item) : HttpEngine::Item(item)
{
  _text = item->_text;
  _tags = item->_tags;
  _condition = item->_condition;
}

QVariant Item::data() const
{
  QString str, newValue;

  if (!_text.isEmpty())
  {
    newValue = _text;

    str = QString("<font size=\"+1\"><b>") + tr("Text:") + "</b> " +
          newValue.replace(" ",(_condition == And)? " <i>" + tr("and") + "</i> " :
                                                    " <i>" + tr("or") + "</i> ") + "</font>";
  }
  else if (_tags.size())
  {
    newValue = _tags.join((_condition == And)? " <i>" + tr("and") + "</i> " :
                                              " <i>" + tr("or") + "</i> ");

    str = QString("<font size=\"+1\"><b>") + tr("Tags:") + "</b> " + newValue;
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
  _sourceUrl.clear();
  photoSize = QSize(-1,-1);
  photoOwner.clear();
  photoDescription.clear();
  photoTitle.clear();
  photoLocation.clear();

  return QUrl();
}

QUrl Item::doPrepareSearch(HttpEngine::Operation & /* op */)
{
  QString searchItem;
  QUrl url("http://api.ipernity.com/api/doc.search/xml");

  url.addQueryItem("api_key",IPERNITY_API_KEY);
  url.addQueryItem("media","photo");
  url.addQueryItem("page",QString::number(pageIndex()));
  url.addQueryItem("per_page","1");

  if (!_tags.isEmpty())
    searchItem = _tags.join((_condition == And)? ",+" : ",");
  else if (!_text.isEmpty())
  {
    searchItem = _text;
    if (_condition == And)
      searchItem.replace(" "," +");
  }
  else
    return QUrl();

  if (_condition == And)
    searchItem = QString("+") + searchItem;

  if (!_tags.isEmpty())
    url.addQueryItem("tags",searchItem);
  else if (!_text.isEmpty())
    url.addQueryItem("text",searchItem);

  return url;
}

bool Item::doProcessSearchResult(const QByteArray &response, int &newPagesCount)
{
  QXmlStreamReader xmlResp(response);
  bool respError = false;

  while (!xmlResp.atEnd() && !respError)
    if (xmlResp.readNext() == QXmlStreamReader::StartElement)
    {
      if (!xmlResp.name().toString().compare("api",Qt::CaseInsensitive))
        respError = xmlResp.attributes().value("status").toString().compare("ok",Qt::CaseInsensitive);
      else if (!xmlResp.name().toString().compare("docs",Qt::CaseInsensitive))
        newPagesCount = xmlResp.attributes().value("total").toString().toInt();
      else if (!xmlResp.name().toString().compare("doc",Qt::CaseInsensitive))
        _photoId = xmlResp.attributes().value("doc_id").toString();
    }

  return (!_photoId.isEmpty() && !_lastPhotoIds.contains(_photoId));
}

QUrl Item::prepareSizeRequest(HttpEngine::Operation & /* op */)
{
  QUrl url("http://api.ipernity.com/api/doc.get/xml");

  url.addQueryItem("api_key",IPERNITY_API_KEY);
  url.addQueryItem("doc_id",_photoId);
  url.addQueryItem("extra","notes,geo");

  return url;
}

QSize Item::doProcessSizeRequestResult(const QByteArray &response)
{
  QXmlStreamReader xmlResp(response);
  bool respError = false;

  while (!xmlResp.atEnd() && !respError)
    if (xmlResp.readNext() == QXmlStreamReader::StartElement)
    {
      if (!xmlResp.name().toString().compare("api",Qt::CaseInsensitive))
        respError = xmlResp.attributes().value("status").toString().compare("ok",Qt::CaseInsensitive);
      else if (!xmlResp.name().toString().compare("thumb",Qt::CaseInsensitive))
      {
        QSize size(xmlResp.attributes().value("w").toString().toInt(),
                   xmlResp.attributes().value("h").toString().toInt());

        if ((size.width() >= photoSize.width()) && (size.height() >= photoSize.height()))
        {
          photoSize = size;
          _photoUrl = xmlResp.attributes().value("url").toString();
        }
      }
      else if (!xmlResp.name().toString().compare("doc",Qt::CaseInsensitive))
      {
        photoTitle = xmlResp.attributes().value("title").toString();
        _sourceUrl = xmlResp.attributes().value("link").toString();
      }
      else if (!xmlResp.name().toString().compare("description",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        photoDescription = xmlResp.text().toString();
      }
      else if (!xmlResp.name().toString().compare("owner",Qt::CaseInsensitive))
        photoOwner = xmlResp.attributes().value("username").toString();
      else if (!xmlResp.name().toString().compare("geo",Qt::CaseInsensitive))
        photoLocation = xmlResp.attributes().value("location").toString();
    }

  return photoSize;
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
  info.location = photoLocation;
  info.sourceUrl = _sourceUrl;

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
    _tags = settings.value(SEARCH_TAGS,QString()).toString().split(" ");
  else if (!settings.value(SEARCH_TEXT,QString()).toString().isEmpty())
    _text = settings.value(SEARCH_TEXT,QString()).toString();

  _condition = static_cast<Condition> (settings.value(CONDITION,And).toInt());

  data = qUncompress(settings.value(LAST_PHOTO_IDS,QByteArray()).toByteArray());

  buf.open(QIODevice::ReadOnly);
  in >> _lastPhotoIds;
  buf.close();
}

void Item::doSaveSettings(QSettings &settings) const
{
  if (_tags.size())
    settings.setValue(SEARCH_TAGS,_tags.join(" "));
  else if (!_text.isEmpty())
    settings.setValue(SEARCH_TEXT,_text);

  settings.setValue(CONDITION,_condition);
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
  Item *ipernityItem = qobject_cast<Item *> (item);

  return ipernityItem && (text() == ipernityItem->text()) &&
                         (tags().join(";") == ipernityItem->tags().join(";")) &&
                         (condition() == ipernityItem->condition());
}

DialogWidget::DialogWidget(XtEngine::Core *core, QWidget *parent) : XtEngine::DialogWidget(core,parent)
{
  QVBoxLayout *mainLayout = new QVBoxLayout;
  QHBoxLayout *topLayout = new QHBoxLayout;
  QHBoxLayout *radioLayout = new QHBoxLayout;

  leSearchFor = new QLineEdit(this);
  leSearchFor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  cbCondition = new QComboBox(this);
  cbCondition->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbCondition->addItem(tr("All of these words"),And);
  cbCondition->addItem(tr("Any of these words"),Or);
  cbCondition->setCurrentIndex(0);

  topLayout->addWidget(new QLabel(tr("Search for:"),this));
  topLayout->addWidget(leSearchFor);
  topLayout->addWidget(cbCondition);

  rbText = new QRadioButton(tr("Full text"),this);
  rbTags = new QRadioButton(tr("Tags only"),this);
  radioLayout->addWidget(rbText);
  radioLayout->addWidget(rbTags);
  radioLayout->addStretch();
  rbText->setChecked(true);

  mainLayout->addLayout(topLayout);
  mainLayout->addLayout(radioLayout);

  setLayout(mainLayout);

  leSearchFor->setFocus();
}

XtEngine::Item *DialogWidget::returnItem()
{
  QMessageBox::warning(this,tr("Ipernity item"),
                       tr("It can show offending or sexual explicit photos"));

  Item *item = qobject_cast<Item *> (core()->newItem());

  if (rbTags->isChecked())
    item->setTags(leSearchFor->text().split(" "));
  else
    item->setText(leSearchFor->text());

  item->setCondition(static_cast<Condition>
                      (cbCondition->itemData(cbCondition->currentIndex()).toInt()));

  return item;
}

void DialogWidget::setupFromItem(XtEngine::Item *item)
{
  if (item)
  {
    Item *ipernityItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit Ipernity item"));

    if (!ipernityItem->text().isEmpty())
    {
      leSearchFor->setText(ipernityItem->text());
      rbText->setChecked(true);
    }
    else
    {
      leSearchFor->setText(ipernityItem->tags().join(" "));
      rbTags->setChecked(true);
    }

    cbCondition->setCurrentIndex(cbCondition->findData(ipernityItem->condition()));
  }
  else
    setWindowTitle(tr("Add Ipernity item"));
}
