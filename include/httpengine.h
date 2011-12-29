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

#ifndef HTTPENGINE_H
#define HTTPENGINE_H

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

#include "ui_http.h"
#include "networkmanager.h"
#include "engine.h"
#include "xtengine.h"

#define PAGE_INDEX "pageIndex"
#define PAGES "pages"

namespace HttpEngine
{
  enum Operation { Get, Post };

  class Item : public XtEngine::Item
  {
    Q_OBJECT

    bool _savePhotos;
    QString _photosPath;

    int _pageIndex;
    int _pages;
    int _consecutiveSearchesFailed;

    void loadState(QSettings &settings, const QString &group);
    void saveState(QSettings &settings, const QString &engineName, int id) const;
    void loadSettings(QSettings &settings, const QString &group);
    void saveSettings(QSettings &settings, const QString &engineName, int id) const;

  protected:
    virtual QUrl doPrepareInit(Operation &op) = 0;
    virtual void doProcessInitResult(const QByteArray & /* response */) { }
    virtual int doCalculateNextIndex(bool randomMode, int currentIndex, int pages) const = 0;
    virtual QUrl doPrepareSearch(Operation &op) = 0;
    virtual bool doProcessSearchResult(const QByteArray &response, int &newPagesCount) = 0;
    virtual QSize doProcessSizeRequestResult(const QByteArray &response) = 0;
    virtual void doProcessDownloadResult(const QFileInfo & /* fileInfo */) { }
    virtual PhotoInfo doProcessInfoCollectResult(const QByteArray &response) = 0;
    virtual void doLoadSettings(QSettings &settings) = 0;
    virtual void doSaveSettings(QSettings &settings) const = 0;
    virtual void doLoadState(QSettings & /* settings */) { }
    virtual void doSaveState(QSettings & /* settings */) const { }
    int pageIndex() const { return _pageIndex; }

  public:
    Item();
    Item(Item *item);
    virtual ~Item() { }

    void resetPages() { _pageIndex = _pages = 0; }

    QUrl prepareInit(Operation &op);
    void processInitResult(const QByteArray &response);
    QUrl prepareSearch(bool randomMode, Operation &op);
    bool processSearchResult(const QByteArray &response);
    virtual QUrl prepareSizeRequest(Operation &op) = 0;
    QSize processSizeRequestResult(const QByteArray &response);
    virtual QUrl prepareDownload(Operation &op) = 0;
    void processDownloadResult(const QFileInfo &fileInfo, bool belowThreshold);
    virtual QUrl prepareInfoCollect(Operation &op) = 0;
    PhotoInfo processInfoCollectResult(const QByteArray &response);

    void setPhotosPath(const QString &photosPath) { _photosPath = photosPath; }
    void setSavePhotos(bool save) { _savePhotos = save; }

    QString photosPath() const { return _photosPath; }
    bool arePhotosSaved() const { return _savePhotos; }

    virtual QString photoUrl() const = 0;
  };

  class Core : public XtEngine::Core
  {
    Q_OBJECT

    bool canceling;
    bool cancelSignaled;
    qint64 _diskSpaceThreshold;
    Network::Manager *initHttp;
    Network::Manager *searchHttp;
    Network::Manager *sizeRequestHttp;
    Network::Manager *downloadHttp;
    Network::Manager *infoCollectHttp;
    QPointer< Network::Reply > currentReply;
    QFileInfo httpFile;
    QNetworkProxy httpProxy;

    QNetworkProxy guessProxy(const QUrl &url);

  private slots:
    void httpInitDone(bool ok, const QByteArray &data);
    void httpSearchDone(bool ok, const QByteArray &data);
    void httpSizeRequestDone(bool ok, const QByteArray &data);
    void httpDownloadDone(bool ok, const QByteArray &data);
    void httpInfoCollectDone(bool ok, const QByteArray &data);

  public:
    Core(const QString &tempStorageDir, QObject *parent = 0);
    virtual ~Core() { }

    void setProxy(const QNetworkProxy &proxy);
    void setFreeDiskSpaceThreshold(qint64 diskSpaceThreshold) { _diskSpaceThreshold = diskSpaceThreshold; }

    QString photoUrl() const { return qobject_cast<Item *> (list.at(currentItemIndex))->photoUrl(); }

  public slots:
    void init();
    void search(bool randomMode);
    void sizeLookup();
    void download();
    void infoCollect();
    void cancel();
  };

  class SettingsWidget : public XtEngine::SettingsWidget, private Ui::HttpSettingsWidget
  {
    Q_OBJECT

    bool isRefreshing;

  private slots:
    void on_tbAdd_clicked();
    void on_tbDel_clicked();
    void on_lvSets_doubleClicked(const QModelIndex &index);
    void on_tbPhotosPath_clicked();
    void updateWidgets(const QItemSelection &selected, const QItemSelection &deselected);
    void on_cbSavePhotos_stateChanged(int state);
    void on_lePhotosPath_textChanged(const QString &text);

  public:
    SettingsWidget(Engine::Core *core, QWidget *parent = 0);
    virtual ~SettingsWidget() { }
  };
}

#endif
