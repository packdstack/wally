/*
TRANSLATOR Photobucket::Item
*/
/*
TRANSLATOR Photobucket::Core
*/
/*
TRANSLATOR Photobucket::DialogWidget
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
#include "photobucket.h"

using namespace Photobucket;

Item::Item() : HttpEngine::Item()
{
  _condition = And;
}

Item::Item(Item *item) : HttpEngine::Item(item)
{
  _text = item->_text;
  _condition = item->_condition;
}

QByteArray Item::hmacSha1(const QByteArray &data, const QByteArray &key)
{
  int i;
  QByteArray ipad, opad, Ki0, Ko0, K0;

  ipad.fill(0x36,64);
  opad.fill(0x5c,64);

  K0 = key;
  if (key.size() > 64)
    K0 = QCryptographicHash::hash(key,QCryptographicHash::Sha1);

  if (K0.size() < 64)
    K0.append(QByteArray().fill(0,64-K0.size()));

  for (i = 0; i < 64; ++i)
  {
    Ki0[i] = K0[i] ^ ipad[i];
    Ko0[i] = K0[i] ^ opad[i];
  }

  Ki0.append(data);
  Ki0 = QCryptographicHash::hash(Ki0,QCryptographicHash::Sha1);
  Ki0.prepend(Ko0);

  return QCryptographicHash::hash(Ki0,QCryptographicHash::Sha1);
}

QVariant Item::data() const
{
  QString str = _text;

  if (!str.isEmpty())
    str = QString("<font size=\"+1\"><b>") + tr("Text:") + "</b> " +
          str.replace(" ",(_condition == And)? " <i>" + tr("and") + "</i> " :
                                               " <i>" + tr("or") + "</i> ") + "</font>";

  return str;
}

int Item::doCalculateNextIndex(bool randomMode, int currentIndex, int pages) const
{
  return (pages)? (currentIndex + ((randomMode)? (qrand() % 5) + 1 : 1)) % pages : 1;
}

QUrl Item::doPrepareInit(HttpEngine::Operation & /* op */)
{
  pbTimestamp = 0;
  _photoUrl.clear();
  _sourceUrl.clear();
  photoTitle.clear();
  photoDescription.clear();
  photoOwner.clear();

  return QUrl("http://api.photobucket.com/time");
}

void Item::doProcessInitResult(const QByteArray &response)
{
  pbTimestamp = response.toInt();
}

QUrl Item::doPrepareSearch(HttpEngine::Operation & /* op */)
{
  if (!pbTimestamp)
    return QUrl();

  QByteArray signature;
  QStringList base, params;
  QString newText = _text;

  newText.replace(" ",(_condition == And)? " and " : " or ");
  QUrl url = QString("http://api.photobucket.com/search/") + newText + "/image";

  QByteArray nonce;
  int nonceLength = (qrand() % 10) + 10;

  while (--nonceLength)
    nonce += qrand() % 10 + 0x30;

  params << (QString("oauth_consumer_key=") + PHOTOBUCKET_API_KEY);
  params << (QString("oauth_nonce=") + nonce);
  params << "oauth_signature_method=HMAC-SHA1";
  params << (QString("oauth_timestamp=") + QString::number(pbTimestamp));
  params << "oauth_version=1.0";
  params << (QString("page=") + QString::number(pageIndex()));
  params << "perpage=1";
  params << "secondaryperpage=0";

  base << "GET";
  base << QUrl::toPercentEncoding(url.toEncoded());
  base << QUrl::toPercentEncoding(params.join("&"));

  signature = hmacSha1(base.join("&").toAscii(),QByteArray(PHOTOBUCKET_SECRET_KEY) + "&");
  params.insert(2,QString("oauth_signature=") + QUrl::toPercentEncoding(signature.toBase64()));

  return QUrl::fromEncoded(QString("%1?%2").arg(url.toString()).arg(params.join("&")).toAscii());
}

bool Item::doProcessSearchResult(const QByteArray &response, int &newPagesCount)
{
  bool respError = false;
  QXmlStreamReader xmlResp(response);

  while ((!xmlResp.atEnd()) && (!respError))
    if (xmlResp.readNext() == QXmlStreamReader::StartElement)
    {
      if (!xmlResp.name().toString().compare("status",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        respError = xmlResp.text().toString().compare("ok",Qt::CaseInsensitive);
      }
      else if (!xmlResp.name().toString().compare("result",Qt::CaseInsensitive))
        newPagesCount = xmlResp.attributes().value("totalpages").toString().toInt();
      else if (!xmlResp.name().toString().compare("url",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        _photoUrl = xmlResp.text().toString();
      }
      else if (!xmlResp.name().toString().compare("albumurl",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        _sourceUrl = xmlResp.text().toString();
      }
      else if (!xmlResp.name().toString().compare("title",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        photoTitle = xmlResp.text().toString();
      }
      else if (!xmlResp.name().toString().compare("description",Qt::CaseInsensitive))
      {
        xmlResp.readNext();
        photoDescription = xmlResp.text().toString();
      }
      else if (!xmlResp.name().toString().compare("media",Qt::CaseInsensitive))
        photoOwner = xmlResp.attributes().value("username").toString();
    }

  return (!respError && _photoUrl.isValid() && !_lastPhotoUrls.contains(_photoUrl));
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
  info.searchString = _text;

  return info;
}

void Item::doLoadSettings(QSettings &settings)
{
  QByteArray data;
  QBuffer buf(&data);
  QDataStream in(&buf);

  _text = settings.value(SEARCH_TEXT,QString()).toString();
  _condition = static_cast<Condition> (settings.value(CONDITION,And).toInt());

  data = qUncompress(settings.value(LAST_PHOTO_URLS,QByteArray()).toByteArray());

  buf.open(QIODevice::ReadOnly);
  in >> _lastPhotoUrls;
  buf.close();
}

void Item::doSaveSettings(QSettings &settings) const
{
  settings.setValue(SEARCH_TEXT,_text);
  settings.setValue(CONDITION,_condition);
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
  Item *photobucketItem = qobject_cast<Item *> (item);

  return photobucketItem && (text() == photobucketItem->text()) &&
                            (condition() == photobucketItem->condition());
}

DialogWidget::DialogWidget(XtEngine::Core *core, QWidget *parent) : XtEngine::DialogWidget(core,parent)
{
  QVBoxLayout *mainLayout = new QVBoxLayout;
  QHBoxLayout *topLayout = new QHBoxLayout;

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

  mainLayout->addLayout(topLayout);
  mainLayout->addStretch();

  setLayout(mainLayout);

  leSearchFor->setFocus();
}

XtEngine::Item *DialogWidget::returnItem()
{
  QMessageBox::warning(this,tr("Photobucket item"),
                       tr("It can show offending or sexual explicit photos"));

  Item *item = qobject_cast<Item *> (core()->newItem());

  item->setText(leSearchFor->text());
  item->setCondition(static_cast<Condition>
                      (cbCondition->itemData(cbCondition->currentIndex()).toInt()));

  return item;
}

void DialogWidget::setupFromItem(XtEngine::Item *item)
{
  if (item)
  {
    Item *photobucketItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit Photobucket item"));

    leSearchFor->setText(photobucketItem->text());
    cbCondition->setCurrentIndex(cbCondition->findData(photobucketItem->condition()));
  }
  else
    setWindowTitle(tr("Add Photobucket item"));
}

void Core::applyWatermark(QPainter *painter, const QRect &rect)
{
  QPixmap pixmap = QPixmap(":/images/pb_watermark").scaled(QSize(rect.width() / 5,
                                                                     rect.height() / 5),
                                                               Qt::KeepAspectRatio,
                                                               Qt::SmoothTransformation);

  painter->drawPixmap(rect.right() - pixmap.width(),
                      rect.bottom() - pixmap.height(),
                      pixmap);
}
