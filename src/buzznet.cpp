/*
TRANSLATOR Buzznet::Item
*/
/*
TRANSLATOR Buzznet::Core
*/
/*
TRANSLATOR Buzznet::DialogWidget
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
#include "buzznet.h"

using namespace Buzznet;

QVariant Item::data() const
{
  QString str;

  if (_tag.size())
    str = QString("<font size=\"+1\"><b>") + tr("Tag:") + "</b> " + _tag + "</font>";

  return str;
}

int Item::doCalculateNextIndex(bool randomMode, int currentIndex, int pages) const
{
  return (pages)? (currentIndex + ((randomMode)? (qrand() % 5) + 1 : 1)) % pages : 0;
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
  if (_tag.size())
  {
    QUrl url(QString("http://www.buzznet.com/tags/") + _tag + "/rss/");

    url.addQueryItem("pagesize","1");
    url.addQueryItem("p",QString::number(pageIndex()));
    url.addQueryItem("mtype","photo");

    return url;
  }
  else
    return QUrl();
}

bool Item::doProcessSearchResult(const QByteArray &response, int &newPagesCount)
{
  bool endItem = false;
  QXmlStreamReader xmlResp(response);

  while (!xmlResp.atEnd())
    if (xmlResp.readNext() == QXmlStreamReader::StartElement)
    {
      if (!xmlResp.prefix().toString().compare("buzznet",Qt::CaseInsensitive) &&
          !xmlResp.name().toString().compare("totalrows",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        newPagesCount = xmlResp.text().toString().toInt();
      }
      else if (!xmlResp.name().toString().compare("item",Qt::CaseInsensitive))
      {
        endItem = false;

        while ((!xmlResp.atEnd()) && (!endItem))
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
              else if (!xmlResp.prefix().toString().compare("media",Qt::CaseInsensitive) &&
                       !xmlResp.name().toString().compare("description",Qt::CaseInsensitive))
              {
                xmlResp.readNext();
                photoDescription = xmlResp.text().toString();
              }
              else if (!xmlResp.prefix().toString().compare("buzznet",Qt::CaseInsensitive) &&
                       !xmlResp.name().toString().compare("orig",Qt::CaseInsensitive))
              {
                xmlResp.readNext();
                _photoUrl = xmlResp.text().toString();
              }
              else if (!xmlResp.prefix().toString().compare("dc",Qt::CaseInsensitive) &&
                       !xmlResp.name().toString().compare("creator",Qt::CaseInsensitive))
              {
                xmlResp.readNext();
                photoOwner = xmlResp.text().toString();
              }
              break;

            case QXmlStreamReader::EndElement:
              endItem = !xmlResp.name().toString().compare("item",Qt::CaseInsensitive);
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
  return QApplication::desktop()->screenGeometry().size();
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
  info.searchString = _tag;

  return info;
}

void Item::doLoadSettings(QSettings &settings)
{
  QByteArray data;
  QBuffer buf(&data);
  QDataStream in(&buf);

  _tag = settings.value(TAG,QString()).toString();

  data = qUncompress(settings.value(LAST_PHOTO_URLS,QByteArray()).toByteArray());

  buf.open(QIODevice::ReadOnly);
  in >> _lastPhotoUrls;
  buf.close();
}

void Item::doSaveSettings(QSettings &settings) const
{
  settings.setValue(TAG,_tag);
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
  Item *buzznetItem = qobject_cast<Item *> (item);

  return buzznetItem && (tag() == buzznetItem->tag());
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

  item->setTag(leSearchFor->text());

  return item;
}

void DialogWidget::setupFromItem(XtEngine::Item *item)
{
  if (item)
  {
    Item *buzznetItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit Buzznet item"));

    leSearchFor->setText(buzznetItem->tag());
  }
  else
    setWindowTitle(tr("Add Buzznet item"));
}
