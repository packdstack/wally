/*
TRANSLATOR Picasa::Item
*/
/*
TRANSLATOR Picasa::Core
*/
/*
TRANSLATOR Picasa::DialogWidget
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
#include "picasa.h"

using namespace Picasa;

Item::Item(Item *item) : HttpEngine::Item(item)
{
  _text = item->_text;
}

QVariant Item::data() const
{
  QString str = _text;

  if (!str.isEmpty())
    str = QString("<font size=\"+1\"><b>") + tr("Text:") + "</b> " + _text + "</font>";

  return str;
}

int Item::doCalculateNextIndex(bool randomMode, int currentIndex, int pages) const
{
  return (pages)? (currentIndex + ((randomMode)? (qrand() % 5) + 1 : 1)) % pages : 1;
}

QUrl Item::doPrepareInit(HttpEngine::Operation & /* op */)
{
  _photoUrl.clear();
  _sourceUrl.clear();
  photoTitle.clear();
  photoDescription.clear();
  photoOwner.clear();
  photoLocation.clear();

  return QUrl();
}

QUrl Item::doPrepareSearch(HttpEngine::Operation & /* op */)
{
  QUrl url("http://picasaweb.google.com/data/feed/base/all");

  url.addQueryItem("kind","photo");
  url.addQueryItem("start-index",QString::number(pageIndex()));
  url.addQueryItem("max-results","1");

  if (_text.isEmpty())
    return QUrl();

  url.addQueryItem("q",_text);

  return url;
}

bool Item::doProcessSearchResult(const QByteArray &response, int &newPagesCount)
{
  bool mediaSection = false, authorSection = false;
  QXmlStreamReader xmlResp(response);

  while (!xmlResp.atEnd())
    if (xmlResp.readNext() == QXmlStreamReader::StartElement)
    {
      if (!xmlResp.prefix().toString().compare("openSearch",Qt::CaseInsensitive) &&
          !xmlResp.name().toString().compare("totalResults",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        newPagesCount = xmlResp.text().toString().toInt();
      }
      else if (!xmlResp.prefix().toString().compare("media",Qt::CaseInsensitive) &&
               !xmlResp.name().toString().compare("group",Qt::CaseInsensitive))
      {
        mediaSection = true;

        while (!xmlResp.atEnd() && mediaSection)
          switch (xmlResp.readNext())
          {
            case QXmlStreamReader::StartElement:
              if (!xmlResp.name().toString().compare("content",Qt::CaseInsensitive))
              {
                _photoUrl = xmlResp.attributes().value("url").toString();
                photoSize = QSize(xmlResp.attributes().value("width").toString().toInt(),
                                  xmlResp.attributes().value("height").toString().toInt());
              }
              else if (!xmlResp.name().toString().compare("description",Qt::CaseInsensitive))
              {
                xmlResp.readNext();
                photoDescription = xmlResp.text().toString();
              }
              else if (!xmlResp.name().toString().compare("title",Qt::CaseInsensitive))
              {
                xmlResp.readNext();
                photoTitle = xmlResp.text().toString();
              }
              break;

            case QXmlStreamReader::EndElement:
              mediaSection = xmlResp.name().toString().compare("group",Qt::CaseInsensitive);
              break;

            default:
              break;
          }
      }
      else if (!xmlResp.prefix().toString().compare("gphoto",Qt::CaseInsensitive) &&
               !xmlResp.name().toString().compare("location",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        photoLocation = xmlResp.text().toString();
      }
      else if (!xmlResp.name().toString().compare("link",Qt::CaseInsensitive))
      {
        if (!xmlResp.attributes().value("rel").toString().compare("alternate",Qt::CaseInsensitive))
          _sourceUrl = xmlResp.attributes().value("href").toString();
      }
      else if (!xmlResp.name().toString().compare("author",Qt::CaseInsensitive))
      {
        authorSection = true;

        while (!xmlResp.atEnd() && authorSection)
          switch (xmlResp.readNext())
          {
            case QXmlStreamReader::StartElement:
              if (!xmlResp.name().toString().compare("name",Qt::CaseInsensitive))
              {
                xmlResp.readNext();
                photoOwner = xmlResp.text().toString();
              }
              break;

            case QXmlStreamReader::EndElement:
              authorSection = xmlResp.name().toString().compare("author",Qt::CaseInsensitive);
              break;

            default:
              break;
          }
      }
    }

  return (_photoUrl.isValid() && !_lastPhotoUrls.contains(_photoUrl));
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
  info.location = photoLocation;
  info.searchString = _text;

  return info;
}

void Item::doLoadSettings(QSettings &settings)
{
  QByteArray data;
  QBuffer buf(&data);
  QDataStream in(&buf);

  _text = settings.value(SEARCH_TEXT,QString()).toString();

  data = qUncompress(settings.value(LAST_PHOTO_URLS,QByteArray()).toByteArray());

  buf.open(QIODevice::ReadOnly);
  in >> _lastPhotoUrls;
  buf.close();
}

void Item::doSaveSettings(QSettings &settings) const
{
  settings.setValue(SEARCH_TEXT,_text);
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
  Item *picasaItem = qobject_cast<Item *> (item);

  return picasaItem && (text() == picasaItem->text());
}

DialogWidget::DialogWidget(XtEngine::Core *core, QWidget *parent) : XtEngine::DialogWidget(core,parent)
{
  QHBoxLayout *mainLayout = new QHBoxLayout;

  leSearchFor = new QLineEdit(this);
  leSearchFor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Maximum);

  mainLayout->addWidget(new QLabel(tr("Search for:"),this));
  mainLayout->addWidget(leSearchFor);

  setLayout(mainLayout);

  leSearchFor->setFocus();
}

XtEngine::Item *DialogWidget::returnItem()
{
  Item *item = qobject_cast<Item *> (core()->newItem());

  item->setText(leSearchFor->text());

  return item;
}

void DialogWidget::setupFromItem(XtEngine::Item *item)
{
  if (item)
  {
    Item *picasaItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit Picasa item"));

    leSearchFor->setText(picasaItem->text());
  }
  else
    setWindowTitle(tr("Add Picasa item"));
}
