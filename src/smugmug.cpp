/*
TRANSLATOR SmugMug::Item
*/
/*
TRANSLATOR SmugMug::Core
*/
/*
TRANSLATOR SmugMug::DialogWidget
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
#include "smugmug.h"

using namespace SmugMug;

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

int Item::doCalculateNextIndex(bool randomMode, int currentIndex, int /* pages */) const
{
  return currentIndex + ((randomMode)? (qrand() % 5) + 1 : 1);
}

QUrl Item::doPrepareInit(HttpEngine::Operation & /* op */)
{
  _photoUrl.clear();
  _sourceUrl.clear();
  photoTitle.clear();
  photoDescription.clear();
  photoOwner.clear();

  return QUrl();
}

QUrl Item::doPrepareSearch(HttpEngine::Operation & /* op */)
{
  QUrl url("http://api.smugmug.com/hack/feed.mg");

  url.addQueryItem("Type","keyword");

  if (_text.isEmpty())
    return QUrl();

  url.addQueryItem("Data",_text);
  url.addQueryItem("format","rss");
  url.addQueryItem("Size","Original");
  url.addQueryItem("start",QString::number(pageIndex()));
  url.addQueryItem("ImageCount","1");

  return url;
}

bool Item::doProcessSearchResult(const QByteArray &response, int & /* newPagesCount */)
{
  bool itemSection = false, mediaGroupSection = false;
  int fileSize = 0;
  QXmlStreamReader xmlResp(response);

  while (!xmlResp.atEnd())
    if ((xmlResp.readNext() == QXmlStreamReader::StartElement) &&
        !xmlResp.name().toString().compare("item",Qt::CaseInsensitive))
    {
      itemSection = true;

      while (!xmlResp.atEnd() && itemSection)
        switch (xmlResp.readNext())
        {
          case QXmlStreamReader::StartElement:
            if (!xmlResp.name().toString().compare("title",Qt::CaseInsensitive))
            {
              xmlResp.readNext();
              photoTitle = xmlResp.text().toString();
            }
            else if (!xmlResp.name().toString().compare("link",Qt::CaseInsensitive))
            {
              xmlResp.readNext();
              _sourceUrl = xmlResp.text().toString();
            }
            else if (!xmlResp.name().toString().compare("description",Qt::CaseInsensitive))
            {
              xmlResp.readNext();
              photoDescription = xmlResp.text().toString();
            }
            else if (!xmlResp.name().toString().compare("author",Qt::CaseInsensitive))
            {
              xmlResp.readNext();
              photoOwner = xmlResp.text().toString();
            }
            else if (!xmlResp.prefix().toString().compare("media",Qt::CaseInsensitive) &&
                    !xmlResp.name().toString().compare("group",Qt::CaseInsensitive))
            {
              mediaGroupSection = true;

              while (!xmlResp.atEnd() && mediaGroupSection)
                switch (xmlResp.readNext())
                {
                  case QXmlStreamReader::StartElement:
                    if (!xmlResp.name().toString().compare("content",Qt::CaseInsensitive) &&
                        !xmlResp.attributes().value("medium").toString().compare("image",Qt::CaseInsensitive) &&
                        (fileSize < xmlResp.attributes().value("fileSize").toString().toInt()) &&
                        ((xmlResp.attributes().value("width").toString().toInt() *
                          xmlResp.attributes().value("height").toString().toInt()) < MAX_PIXEL_NUMBER))
                    {
                      fileSize = xmlResp.attributes().value("fileSize").toString().toInt();
                      _photoUrl = xmlResp.attributes().value("url").toString();
                      photoSize = QSize(xmlResp.attributes().value("width").toString().toInt(),
                                        xmlResp.attributes().value("height").toString().toInt());
                    }
                    break;

                  case QXmlStreamReader::EndElement:
                    mediaGroupSection = xmlResp.name().toString().compare("group",Qt::CaseInsensitive);
                    break;

                  default:
                    break;
                }
            }
            break;

          case QXmlStreamReader::EndElement:
            itemSection = xmlResp.name().toString().compare("item",Qt::CaseInsensitive);
            break;

          default:
            break;
      }
    }

  if (!_photoUrl.isValid())
    resetPages();

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
  QTextDocument doc;
  PhotoInfo info;

  info.title = photoTitle;
  doc.setHtml(photoDescription);
  info.description = doc.toPlainText();
  info.owner = photoOwner;
  info.sourceUrl = _sourceUrl;
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
  Item *smugmugItem = qobject_cast<Item *> (item);

  return smugmugItem && (text() == smugmugItem->text());
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
    Item *smugMugItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit SmugMug item"));

    leSearchFor->setText(smugMugItem->text());
  }
  else
    setWindowTitle(tr("Add SmugMug item"));
}
