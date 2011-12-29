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

#ifndef HISTORY_H
#define HISTORY_H

#include <QtCore>
#include <QtGui>
#include <QtSql>
#include <QtNetwork>

#include "networkmanager.h"
#include "defs.h"
#include "gui.h"

namespace History
{
  class SortModel : public QSortFilterProxyModel
  {
  protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

  public:
    SortModel(QObject *parent = 0) : QSortFilterProxyModel(parent) { }
    virtual ~SortModel() { }
  };

  class EngineQueryModel : public QSqlQueryModel
  {
    Q_OBJECT

  public:
    EngineQueryModel(const QSqlDatabase &db, QObject *parent = 0);
    virtual ~EngineQueryModel() { }

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const;
  };

  class TagsQueryModel : public QSqlQueryModel
  {
    Q_OBJECT

    QSqlDatabase _db;

  public:
    TagsQueryModel(const QSqlDatabase &db, QObject *parent = 0);
    virtual ~TagsQueryModel() { }

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const;

    void selectEngine(const QString &engineName);
  };

  class PhotosQueryModel : public QSqlQueryModel
  {
    Q_OBJECT

    QSqlQuery currentQuery;
    QSqlDatabase _db;
    QTimer sizeHintTimer;
    mutable QMap<int, QSize> sizeHints;

    QString formatCaption(const QSqlRecord &itemRecord) const;
    QWidget *createWidget(const QSqlRecord &itemRecord) const;

  private slots:
    void calculateSizeHints();

  protected:
    void queryChange();

  public:
    PhotosQueryModel(const QSqlDatabase &db, QObject *parent = 0);
    virtual ~PhotosQueryModel() { }

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const;

    void selectData(const QString &engineName, const QString &tags);

  signals:
    void updateViewItem(const QModelIndex &index);
  };

  class PhotoThumbnail : public QItemDelegate
  {
    Q_OBJECT

    QPalette _palette;

  public:
    PhotoThumbnail(const QPalette &palette, QObject *parent = 0) :
      QItemDelegate(parent), _palette(palette) { }
    virtual ~PhotoThumbnail() { }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  };

  class Dialog : public Gui::Dialog
  {
    Q_OBJECT

    enum Request { reqWallpaper, reqFile, reqView };

    Request request;
    QSqlDatabase _db;
    QString _tempStorageDir;

    QString fileName;
    Network::Manager *http;
    QNetworkProxy httpProxy;
    QProgressDialog *httpProgressDialog;
    QProgressDialog *newHttpProgressDialog(QWidget *parent = 0);
    void initHttp();

    EngineQueryModel *enginesQueryModel;
    SortModel *sortedEnginesQueryModel;
    TagsQueryModel *tagsQueryModel;
    SortModel *sortedTagsQueryModel;
    PhotosQueryModel *photosQueryModel;
    QItemSelectionModel *sortedEnginesSelectionModel;
    QItemSelectionModel *sortedTagsSelectionModel;
    QItemSelectionModel *photosSelectionModel;
    QListView *enginesView;
    QListView *tagsView;
    QListView *photosView;
    QAction *exploreSourceAction;
    void initializeModelsAndViews();

    void initializeActions();

    QNetworkProxy guessProxy(const QUrl &url);

  private slots:
    void httpDone(bool ok, const QByteArray &data);
    void httpDownloading(qint64 done, qint64 total);
    void changeTagsList(const QModelIndex &current, const QModelIndex &previous);
    void changePhotosView(const QModelIndex &current, const QModelIndex &previous);
    void execSavePhoto();
    void execSetAsBackground();
    void execExploreImageSource();
    void execViewPhoto();
    void execGetExifInfo();
    void updateContextMenuState(const QModelIndex &current, const QModelIndex &previous);

  public:
    Dialog(const QSqlDatabase &db, const QString &tempStorageDir, QWidget *parent = 0);
    virtual ~Dialog() { }

    void setProxy(const QNetworkProxy &proxy);

  signals:
    void photoDownloaded(bool ok, const QFileInfo &localFile);
    void infoCollected(const PhotoInfo &info);
    void changePhoto(const QString &engineName);
  };
}

Q_DECLARE_METATYPE(QWidget *)

#endif
