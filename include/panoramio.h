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

#ifndef PANORAMIO_H
#define PANORAMIO_H

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

#include "idcache.h"
#include "gui.h"
#include "httpengine.h"

#define PANORAMIO_ENGINE_NAME "Panoramio"

#define MIN_LONGITUDE "minLongitude"
#define MAX_LONGITUDE "maxLongitude"
#define MIN_LATITUDE "minLatitude"
#define MAX_LATITUDE "maxLatitude"
#define SEARCH_ORDER "searchOrder"
#define REQUESTED_SIZE "requestedSize"
#define LAST_PHOTO_IDS "lastPhotoIds"

namespace Panoramio
{
  enum RequestedSize { Original = 0, Medium, Small, Thumbnail, Square, MiniSquare };
  enum SearchOrder { Popularity = 0, UploadDate };

  const QString searchOrderStrings[] = { "popularity", "upload_date" };
  const QString requestedSizeStrings[] = { "original", "medium", "small",
                                           "thumbnail", "square", "mini_square" };

  class Item : public HttpEngine::Item
  {
    Q_OBJECT

    RequestedSize _requestedSize;
    SearchOrder _searchOrder;
    double _minLatitude;
    double _maxLatitude;
    double _minLongitude;
    double _maxLongitude;

    QString _photoId;
    IdCache<QString> _lastPhotoIds;
    QString photoOwner;
    QString photoTitle;
    QString photoLocation;
    QUrl _photoUrl;
    QUrl _sourceUrl;
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

    bool isValidItem() const { return (_minLatitude < _maxLatitude) && (_minLongitude < _maxLongitude); }

  public:
    Item();
    Item(Item *item);
    virtual ~Item() { }

    void setRequestedSize(RequestedSize requestedSize) { _requestedSize = requestedSize; }
    void setSearchOrder(SearchOrder searchOrder) { _searchOrder = searchOrder; }
    void setMinLatitude(double minLatitude) { _minLatitude = minLatitude; }
    void setMaxLatitude(double maxLatitude) { _maxLatitude = maxLatitude; }
    void setMinLongitude(double minLongitude) { _minLongitude = minLongitude; }
    void setMaxLongitude(double maxLongitude) { _maxLongitude = maxLongitude; }

    RequestedSize requestedSize() const { return _requestedSize; }
    SearchOrder searchOrder() const { return _searchOrder; }
    double minLatitude() const { return _minLatitude; }
    double maxLatitude() const { return _maxLatitude; }
    double minLongitude() const { return _minLongitude; }
    double maxLongitude() const { return _maxLongitude; }

    QString photoUrl() const { return _photoUrl.toString(); }

    bool equalTo(Engine::Item *item) const;
  };

  class DialogWidget : public XtEngine::DialogWidget
  {
    Q_OBJECT

    bool checkingValues;

    QDoubleSpinBox *dsbMinLongitude;
    QDoubleSpinBox *dsbMaxLongitude;
    QDoubleSpinBox *dsbMinLatitude;
    QDoubleSpinBox *dsbMaxLatitude;
    QComboBox *cbRequestedSize;
    QComboBox *cbSearchOrder;

    XtEngine::Item *returnItem();
    void setupFromItem(XtEngine::Item *item = 0);
    bool validateInput();

  private slots:
    void correctValues();
    void launchMapViewer();

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

    QPixmap buttonPixmap() const { return QPixmap(":/images/panoramio"); }
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

    QString name() const { return PANORAMIO_ENGINE_NAME; }

    Engine::SettingsWidget *newSettingsWidget(QWidget *parent = 0) { return new SettingsWidget(this,parent); }

  public:
    Core(const QString &tempStorageDir, QObject *parent = 0) : HttpEngine::Core(tempStorageDir,parent) { }
    virtual ~Core() { }
  };
}

#endif
