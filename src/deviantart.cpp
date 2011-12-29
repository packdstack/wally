/*
TRANSLATOR DeviantArt::Item
*/
/*
TRANSLATOR DeviantArt::Core
*/
/*
TRANSLATOR DeviantArt::DialogWidget
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
#include "deviantart.h"

using namespace DeviantArt;

QVariant Item::data() const
{
  QString str;

  if (_tag.size())
  {
    str = QString("<font size=\"+1\"><b>") + tr("Tag:") + "</b> " + _tag + "</font>" +
          "<br>&nbsp;&nbsp;<i>" + tr("Adult filter:") + " ";

    switch (_adultFilter)
    {
      case Off:
        str += tr("Off");
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
  return (pages)? (currentIndex + ((randomMode)? (qrand() % 5) + 1 : 1)) % pages : 0;
}

QUrl Item::doPrepareInit(HttpEngine::Operation & /* op */)
{
  _photoUrl.clear();
  _sourceUrl.clear();
  photoTitle.clear();
  photoDescription.clear();
  photoOwner.clear();
  photoSize = QSize(-1,-1);

  return QUrl();
}

QUrl Item::doPrepareSearch(HttpEngine::Operation & /* op */)
{
  if (_tag.size())
  {
    QUrl url("http://backend.deviantart.com/rss.xml");

    url.addQueryItem("q",_tag);
    url.addQueryItem("offset",QString::number(pageIndex()));

    return url;
  }
  else
    return QUrl();
}

bool Item::doProcessSearchResult(const QByteArray &response, int &newPagesCount)
{
  bool endItem = false, found = false, isAdultContent = false;
  QXmlStreamReader xmlResp(response);

  while (!xmlResp.atEnd() && !found)
    if (xmlResp.readNext() == QXmlStreamReader::StartElement)
    {
      if (!xmlResp.name().toString().compare("item",Qt::CaseInsensitive))
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
              else if (!xmlResp.prefix().toString().compare("media",Qt::CaseInsensitive) &&
                       !xmlResp.name().toString().compare("content",Qt::CaseInsensitive) &&
                       !xmlResp.attributes().value("medium").toString().compare("image",Qt::CaseInsensitive))
              {
                _photoUrl = xmlResp.attributes().value("url").toString();
                photoSize.setWidth(xmlResp.attributes().value("width").toString().toInt());
                photoSize.setHeight(xmlResp.attributes().value("height").toString().toInt());
              }
              else if (!xmlResp.prefix().toString().compare("media",Qt::CaseInsensitive) &&
                       !xmlResp.name().toString().compare("rating",Qt::CaseInsensitive))
              {
                xmlResp.readNext();
                isAdultContent = !xmlResp.text().toString().compare("adult",Qt::CaseInsensitive);
              }
              break;

            case QXmlStreamReader::EndElement:
              endItem = !xmlResp.name().toString().compare("item",Qt::CaseInsensitive);
              found = true;
              break;

            default:
              break;
          }
      }
    }

  newPagesCount = (!found)? 1 : newPagesCount + 1;

  return (found && _photoUrl.isValid() && !_lastPhotoUrls.contains(_photoUrl) &&
          (!isAdultContent || (_adultFilter == Off)));
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
  info.searchString = _tag;

  return info;
}

void Item::doLoadSettings(QSettings &settings)
{
  QByteArray data;
  QBuffer buf(&data);
  QDataStream in(&buf);

  _tag = settings.value(TAG,QString()).toString();
  setAdultFilter(static_cast<AdultFilter> (settings.value(ADULT_FILTER,Strict).toInt()));

  data = qUncompress(settings.value(LAST_PHOTO_URLS,QByteArray()).toByteArray());

  buf.open(QIODevice::ReadOnly);
  in >> _lastPhotoUrls;
  buf.close();
}

void Item::doSaveSettings(QSettings &settings) const
{
  settings.setValue(TAG,_tag);
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
  Item *deviantArtItem = qobject_cast<Item *> (item);

  return deviantArtItem && (tag() == deviantArtItem->tag()) &&
                           (adultFilter() == deviantArtItem->adultFilter());
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

  cbAdultFilter = new QComboBox(this);
  cbAdultFilter->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbAdultFilter->addItem(tr("Strict"),Strict);
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

  item->setTag(leSearchFor->text());
  item->setAdultFilter(static_cast<AdultFilter>
                        (cbAdultFilter->itemData(cbAdultFilter->currentIndex()).toInt()));

  if (item->adultFilter() == Off)
    QMessageBox::warning(this,tr("deviantART item"),
                         tr("Unfiltered content can show offending or sexual explicit photos"));

  return item;
}

void DialogWidget::setupFromItem(XtEngine::Item *item)
{
  if (item)
  {
    Item *deviantArtItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit deviantART item"));

    leSearchFor->setText(deviantArtItem->tag());
    cbAdultFilter->setCurrentIndex(cbAdultFilter->findData(deviantArtItem->adultFilter()));
  }
  else
    setWindowTitle(tr("Add deviantART item"));
}
