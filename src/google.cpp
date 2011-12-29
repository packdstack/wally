/*
TRANSLATOR Google::Item
*/
/*
TRANSLATOR Google::Core
*/
/*
TRANSLATOR Google::DialogWidget
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

#include "json.h"
#include "httpengine.h"
#include "google.h"

using namespace Google;

Item::Item() : HttpEngine::Item(), _lastPhotoIds(5)
{
  _adultFilter = Strict;
}

Item::Item(Item *item) : HttpEngine::Item(item), _lastPhotoIds(5)
{
  _text = item->_text;
  _adultFilter = item->_adultFilter;
}

QVariant Item::data() const
{
  QString str;

  if (_text.size())
  {
    str = QString("<font size=\"+1\"><b>") + tr("Text:") + "</b> " + _text + "</font>" +
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
  QUrl url("http://ajax.googleapis.com/ajax/services/search/images");

  url.addQueryItem("key",GOOGLE_API_KEY);

  if (_text.size())
    url.addQueryItem("q",_text);
  else
    return QUrl();

  url.addQueryItem("v","1.0");

  switch (_adultFilter)
  {
    case Off:
      url.addQueryItem("safe","off");
      break;

    case Moderate:
      url.addQueryItem("safe","moderate");
      break;

    case Strict:
    default:
      url.addQueryItem("safe","active");
      break;
  }

  url.addQueryItem("rsz","1");
  url.addQueryItem("start",QString::number(pageIndex()));
  url.addQueryItem("imgsz","large|xlarge|xxlarge|huge");
  url.addQueryItem("imgtype","photo");

  return url;
}

bool Item::doProcessSearchResult(const QByteArray &response, int &newPagesCount)
{
  QByteArray resp = response;
  QBuffer buffer(&resp);

  buffer.open(QIODevice::ReadOnly);

  try
  {
    JSON::Reader reader(&buffer);

    JSON::Object *rootObject, *object;
    JSON::Array *array;
    JSON::Value *value;

    rootObject = dynamic_cast<JSON::Object *> (reader.element());

    if (!rootObject)
    {
      qDebug() << metaObject()->className() << ": cannot find root object";
      return false;
    }

    if (( (value = dynamic_cast<JSON::Value *> (rootObject->value("responseDetails"))) ) &&
        !value->toString().compare("out of range start"))
    {
      newPagesCount = 1;
      return false;
    }

    rootObject = dynamic_cast<JSON::Object *> (rootObject->value("responseData"));

    if (!rootObject)
    {
      qDebug() << metaObject()->className() << ": cannot find responseData object";
      return false;
    }

    array = dynamic_cast<JSON::Array *> (rootObject->value("results"));

    if (!array || !array->size())
    {
      qDebug() << metaObject()->className() << ": cannot find photos array";
      return false;
    }

    object = dynamic_cast<JSON::Object *> (array->at(0));

    if (!object)
    {
      qDebug() << metaObject()->className() << ": cannot find object inside array";
      return false;
    }

    value = dynamic_cast<JSON::Value *> (object->value("imageId"));
    if (value)
      _photoId = value->toString();
    else
      qDebug() << metaObject()->className() << ": cannot find imageId";

    value = dynamic_cast<JSON::Value *> (object->value("url"));
    if (value)
      _photoUrl = value->toString();
    else
    {
      value = dynamic_cast<JSON::Value *> (object->value("unescapedUrl"));
      if (value)
        _photoUrl = value->toString();
      else
        qDebug() << metaObject()->className() << ": cannot find url/unescapedUrl";
    }

    value = dynamic_cast<JSON::Value *> (object->value("originalContextUrl"));
    if (value)
      _sourceUrl = value->toString();
    else
      qDebug() << metaObject()->className() << ": cannot find originalContextUrl";

    value = dynamic_cast<JSON::Value *> (object->value("title"));
    if (value)
      photoTitle = value->toString();
    else
    {
      value = dynamic_cast<JSON::Value *> (object->value("titleNoFormatting"));
      if (value)
        photoTitle = value->toString();
      else
        qDebug() << metaObject()->className() << ": cannot find title/titleNoFormatting";
    }

    value = dynamic_cast<JSON::Value *> (object->value("width"));
    if (value)
      photoSize.setWidth(value->toInt());
    else
      qDebug() << metaObject()->className() << ": cannot find width";

    value = dynamic_cast<JSON::Value *> (object->value("height"));
    if (value)
      photoSize.setHeight(value->toInt());
    else
      qDebug() << metaObject()->className() << ": cannot find height";

    value = dynamic_cast<JSON::Value *> (object->value("content"));
    if (value)
      photoDescription = value->toString();
    else
    {
      value = dynamic_cast<JSON::Value *> (object->value("contentNoFormatting"));
      if (value)
        photoDescription = value->toString();
      else
        qDebug() << metaObject()->className() << ": cannot find content/contentNoFormatting";
    }

    object = dynamic_cast<JSON::Object *> (rootObject->value("cursor"));
    value = dynamic_cast<JSON::Value *> (object->value("estimatedResultCount"));
    newPagesCount = value->toInt();
  }
  catch (const QString &message)
  {
    qDebug() << message;
  }

  return (!_photoId.isEmpty() && !_lastPhotoIds.contains(_photoId));
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
  info.searchString = _text;

  return info;
}

void Item::doLoadSettings(QSettings &settings)
{
  QByteArray data;
  QBuffer buf(&data);
  QDataStream in(&buf);

  _text = settings.value(TEXT,QString()).toString();
  setAdultFilter(static_cast<AdultFilter> (settings.value(ADULT_FILTER,Strict).toInt()));

  data = qUncompress(settings.value(LAST_PHOTO_IDS,QByteArray()).toByteArray());

  buf.open(QIODevice::ReadOnly);
  in >> _lastPhotoIds;
  buf.close();
}

void Item::doSaveSettings(QSettings &settings) const
{
  settings.setValue(TEXT,_text);
  settings.setValue(ADULT_FILTER,static_cast<int> (_adultFilter));
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
  Item *googleItem = qobject_cast<Item *> (item);

  return googleItem && (text() == googleItem->text()) &&
                       (adultFilter() == googleItem->adultFilter());
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

  item->setText(leSearchFor->text());
  item->setAdultFilter(static_cast<AdultFilter>
                        (cbAdultFilter->itemData(cbAdultFilter->currentIndex()).toInt()));

  if (item->adultFilter() == Off)
    QMessageBox::warning(this,tr("Google item"),
                         tr("Unfiltered content can show offending or sexual explicit photos"));

  return item;
}

void DialogWidget::setupFromItem(XtEngine::Item *item)
{
  if (item)
  {
    Item *googleItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit Google item"));

    leSearchFor->setText(googleItem->text());
    cbAdultFilter->setCurrentIndex(cbAdultFilter->findData(googleItem->adultFilter()));
  }
  else
    setWindowTitle(tr("Add Google item"));
}
