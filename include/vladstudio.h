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

#ifndef VLADSTUDIO_H
#define VLADSTUDIO_H

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

#include "idcache.h"
#include "httpengine.h"

#define VLADSTUDIO_ENGINE_NAME "Vladstudio"

#define CATEGORY "category"
#define VIEW_ORDER "viewOrder"
#define ORDER_DIRECTION "orderDirection"
#define LAST_PHOTO_IDS "lastPhotoIds"

namespace Vladstudio
{
  enum Category { All = 0, AbstractArt = 1, Creatures = 2, Illustrations = 3, Photos = 4 };
  enum ViewOrder { ById, ByView };
  enum OrderDirection { Ascending, Descending };

  class Item : public HttpEngine::Item
  {
    Q_OBJECT

    Category _category;
    ViewOrder _viewOrder;
    OrderDirection _orderDirection;

    QString _photoId;
    QString photoTitle;
    QString photoDescription;
    IdCache<QString> _lastPhotoIds;
    QUrl _photoUrl;
    QUrl _sourceUrl;
    QString photoOwner;
    QSize photoSize;

    int doCalculateNextIndex(bool randomMode, int currentIndex, int pages) const;
    QUrl doPrepareInit(HttpEngine::Operation &op);
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
    bool isValidItem() const { return true; }

    static QString categoryToString(Category category);

  public:
    Item();
    Item(Item *item);
    virtual ~Item() { }

    void setCategory(Category category) { _category = category; }
    void setViewOrder(ViewOrder viewOrder) { _viewOrder = viewOrder; }
    void setOrderDirection(OrderDirection orderDirection) { _orderDirection = orderDirection; }

    Category category() const { return _category; }
    ViewOrder viewOrder() const { return _viewOrder; }
    OrderDirection orderDirection() const { return _orderDirection; }

    QString photoUrl() const { return _photoUrl.toString(); }

    bool equalTo(Engine::Item *item) const;
  };

  class DialogWidget : public XtEngine::DialogWidget
  {
    Q_OBJECT

    QComboBox *cbCategory;
    QComboBox *cbViewOrder;
    QComboBox *cbOrderDirection;

    XtEngine::Item *returnItem();
    void setupFromItem(XtEngine::Item *item = 0);

  public:
    DialogWidget(XtEngine::Core *core, QWidget *parent = 0);
    virtual ~DialogWidget() { }
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

    QPixmap buttonPixmap() const { return QPixmap(":/images/vladstudio"); }
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

    QString name() const { return VLADSTUDIO_ENGINE_NAME; }

    Item *newItem(const QVariant & /* data */ = QVariant()) { return new Item; }
    Item *newItem(Engine::Item *item) { return new Item(qobject_cast<Item *> (item)); }

    Engine::SettingsWidget *newSettingsWidget(QWidget *parent = 0) { return new SettingsWidget(this,parent); }

  public:
    Core(const QString &tempStorageDir, QObject *parent = 0) : HttpEngine::Core(tempStorageDir,parent) { }
    virtual ~Core() { }
  };
}

#endif
