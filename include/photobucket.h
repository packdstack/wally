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

#ifndef PHOTOBUCKET_H
#define PHOTOBUCKET_H

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

#include "idcache.h"
#include "httpengine.h"

#define PHOTOBUCKET_API_KEY "149827768"
#define PHOTOBUCKET_SECRET_KEY "0b63a925baaead519308130cbf11028b"

#define PHOTOBUCKET_ENGINE_NAME "Photobucket"

#define SEARCH_TEXT "text"
#define CONDITION "condition"
#define LAST_PHOTO_URLS "lastPhotoUrls"

namespace Photobucket
{
  enum Condition { And = 0, Or };

  class Item : public HttpEngine::Item
  {
    Q_OBJECT

    int pbTimestamp;
    QUrl _photoUrl;
    IdCache<QUrl> _lastPhotoUrls;
    QUrl _sourceUrl;
    QString photoOwner;
    QString photoDescription;
    QString photoTitle;
    QString _text;
    Condition _condition;

    QByteArray hmacSha1(const QByteArray &data, const QByteArray &key);

    int doCalculateNextIndex(bool randomMode, int currentIndex, int pages) const;
    QUrl doPrepareInit(HttpEngine::Operation &op);
    void doProcessInitResult(const QByteArray &response);
    QUrl doPrepareSearch(HttpEngine::Operation &op);
    bool doProcessSearchResult(const QByteArray &response, int &newPagesCount);
    QUrl prepareSizeRequest(HttpEngine::Operation &op);
    QSize doProcessSizeRequestResult(const QByteArray &response);
    QUrl prepareDownload(HttpEngine::Operation &op);
    QUrl prepareInfoCollect(HttpEngine::Operation &op);
    PhotoInfo doProcessInfoCollectResult(const QByteArray &response);

    void doLoadSettings(QSettings &settings);
    void doSaveSettings(QSettings &settings) const;
    void doSaveState(QSettings &settings) const;

    QVariant data() const;
    bool isValidItem() const { return _text.size(); }

  public:
    Item();
    Item(Item *item);
    virtual ~Item() { }

    void setText(const QString &text) { _text = text; }
    void setCondition(Condition condition) { _condition = condition; }

    QString text() const { return _text; }
    Condition condition() const { return _condition; }

    QString photoUrl() const { return _photoUrl.toString(); }

    bool equalTo(Engine::Item *item) const;
  };

  class DialogWidget : public XtEngine::DialogWidget
  {
    Q_OBJECT

    QLineEdit *leSearchFor;
    QComboBox *cbCondition;

    XtEngine::Item *returnItem();
    void setupFromItem(XtEngine::Item *item = 0);

  public:
    DialogWidget(XtEngine::Core *core, QWidget *parent = 0);
    virtual ~DialogWidget() { }

    QSize sizeHint() const { return QSize(500,height()); }
  };

  class Dialog : public XtEngine::Dialog
  {
    Q_OBJECT

    XtEngine::DialogWidget *newDialogWidget(XtEngine::Core *core) { return new DialogWidget(core,this); }

  public:
    Dialog(XtEngine::Core *core, QWidget *parent = 0) : XtEngine::Dialog(core,parent) { }
    virtual ~Dialog() { }
  };

  class SettingsWidget : public HttpEngine::SettingsWidget
  {
    Q_OBJECT

    QPixmap buttonPixmap() const { return QPixmap(":/images/photobucket"); }
    QIcon buttonIcon() const { return QIcon(); }
    QString buttonText() const { return QString(); }

    XtEngine::Dialog *newDialog() { return new Dialog(qobject_cast<XtEngine::Core *> (core()),this); }

  public:
    SettingsWidget(Engine::Core *core, QWidget *parent = 0) : HttpEngine::SettingsWidget(core,parent) { }
    virtual ~SettingsWidget() { }
  };

  class Core : public HttpEngine::Core
  {
    Q_OBJECT

    Item *newItem(const QVariant & /* data */ = QVariant()) { return new Item; }
    Item *newItem(Engine::Item *item) { return new Item(qobject_cast<Item *> (item)); }

    QString name() const { return PHOTOBUCKET_ENGINE_NAME; }

    QAbstractButton *newButton(QWidget *parent = 0);
    Engine::SettingsWidget *newSettingsWidget(QWidget *parent = 0) { return new SettingsWidget(this,parent); }

    void applyWatermark(QPainter *painter, const QRect &rect);

  public:
    Core(const QString &tempStorageDir, QObject *parent = 0) : HttpEngine::Core(tempStorageDir,parent) { }
    virtual ~Core() { }
  };
}

#endif
