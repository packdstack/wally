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

#ifndef ENGINE_H
#define ENGINE_H

#include <QtCore>
#include <QtGui>

#define ENGINES_SECTION "Engines"
#define ENGINE_ACTIVE "Active"

#define ITEM_VIEW_MARGIN 3

#include "defs.h"

namespace Engine
{
  class SettingsWidget;

  class Item : public QObject
  {
    Q_OBJECT

  public:
    virtual ~Item() { }
    virtual QVariant data() const = 0;

    virtual void loadSettings(QSettings &settings, const QString &group) = 0;
    virtual void saveSettings(QSettings &settings, const QString &engineName, int id) const = 0;
    virtual void loadState(QSettings &settings, const QString &group) = 0;
    virtual void saveState(QSettings &settings, const QString &engineName, int id) const = 0;

    virtual bool equalTo(Item *item) const = 0;
  };

  class Core : public QAbstractItemModel
  {
    Q_OBJECT

    bool _active;
    QItemSelectionModel *_selectionModel;
    QString _tempStorageDir;

  protected:
    QList<Item *> sourceList;
    QList<Item *> list;

    virtual void doRevert() { }
    virtual void doSubmit() { }

  public:
    Core(const QString &tempStorageDir, QObject *parent = 0);
    virtual ~Core();

    QString tempStorageDir() const { return _tempStorageDir; }

    QModelIndex parent(const QModelIndex & /* child */) const { return QModelIndex(); }
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    int rowCount(const QModelIndex & /* parent */ = QModelIndex()) const { return list.size(); }
    int columnCount(const QModelIndex & /* parent */ = QModelIndex()) const { return 1; }
    QVariant data(const QModelIndex &index, int role) const;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    bool removeRows(const QItemSelectionModel *selectionModel);
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    QItemSelectionModel *selectionModel() const { return _selectionModel; }

    void edit();
    void revert();
    bool submit();

    void setActive(bool active) { _active = active; }
    bool isActive() const { return _active; }

    virtual void loadState(QSettings & /* settings */) { }
    virtual void saveState(QSettings &settings) const;
    virtual void loadSettings(QSettings &settings);
    virtual void saveSettings(QSettings &settings) const;
    virtual QString name() const = 0;

    virtual QString photoUrl() const { return QString(); }

    virtual Item *newItem(const QVariant &data = QVariant()) = 0;
    virtual Item *newItem(Item *item) = 0;
    virtual SettingsWidget *newSettingsWidget(QWidget *parent = 0) = 0;

    Core *operator<<(Item *item);

    virtual void applyWatermark(QPainter * /* painter */, const QRect & /* rect */) { }

  signals:
    void initCompleted();
    void searchCompleted(bool hasData);
    void sizeLookupCompleted(const QSize &size);
    void downloadCompleted(bool ok, const QFileInfo &localFile);
    void infoCollectCompleted(const PhotoInfo &info);
    void cancelCompleted();

  public slots:
    virtual void init() = 0;
    virtual void search(bool randomMode) = 0;
    virtual void sizeLookup() = 0;
    virtual void download() = 0;
    virtual void infoCollect() = 0;
    virtual void cancel() = 0;
  };

  class SettingsWidget : public QWidget
  {
    Q_OBJECT

    Core *_core;

  public:
    SettingsWidget(Core *core, QWidget *parent = 0);
    virtual ~SettingsWidget() { }

    Core *core() const { return _core; }

    virtual QPixmap buttonPixmap() const = 0;
    virtual QIcon buttonIcon() const = 0;
    virtual QString buttonText() const = 0;

  signals:
    void settingsModified();
  };
}

Q_DECLARE_METATYPE(Engine::Item *)
Q_DECLARE_METATYPE(Engine::Core *)

#endif
