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

#ifndef FILES_H
#define FILES_H

#include <QtCore>
#include <QtGui>

#include "ui_files.h"
#include "engine.h"

#define FILES_ENGINE_NAME "Files"

#define LAST_INDEX "lastIndex"

namespace Files
{
  class Item : public Engine::Item, public QFileInfo
  {
    Q_OBJECT

    QVariant data() const;
    void loadSettings(QSettings & /* settings */, const QString & /* group */) { }
    void saveSettings(QSettings & /* settings */, const QString & /* engineName */ , int /* id */) const { }
    void loadState(QSettings & /* settings */, const QString & /* group */) { }
    void saveState(QSettings & /* settings */, const QString & /* engineName */, int /* id */) const { }

  public:
    Item() { }
    Item(Item *item) : QFileInfo(*item) { }
    Item(const QFileInfo &fileInfo) : QFileInfo(fileInfo) { }
    virtual ~Item() { }

    bool equalTo(Engine::Item *item) const { return *this == *qobject_cast<Item *> (item); }
  };

  class LabelPreview : public QLabel
  {
    Q_OBJECT

  public:
    LabelPreview(QWidget *parent = 0, Qt::WindowFlags f = 0);
    virtual ~LabelPreview() { }

    void setPixmap(const QPixmap &pixmap);

    QSize sizeHint() const { return QSize(250,250); }

  public slots:
    void clear();
  };

  class SettingsWidget : public Engine::SettingsWidget, private Ui::FilesSettingsWidget
  {
    Q_OBJECT

    LabelPreview *lblPreview;
    bool dontChangePreview;

    QPixmap buttonPixmap() const { return QPixmap(); }
    QIcon buttonIcon() const { return QIcon(":/images/files"); }
    QString buttonText() const { return tr("Files"); }

  private slots:
    void on_tbAdd_clicked(bool);
    void on_tbFolderAdd_clicked(bool);
    void on_tbDel_clicked(bool);
    void on_lvPhotos_doubleClicked(const QModelIndex &index);
    void on_tbMoveUp_clicked(bool);
    void on_tbMoveDown_clicked(bool);
    void updateWidgets(const QItemSelection &selected, const QItemSelection &deselected);

  protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

  public:
    SettingsWidget(Engine::Core *core, QWidget *parent = 0);
    virtual ~SettingsWidget() { }
  };

  class Core : public Engine::Core
  {
    Q_OBJECT

    int selectedIndex;
    QList<int> toBeViewed;

    void buildRandomIndexesList();

    void doSubmit();
    void doSaveState(QSettings &settings) const;

    QString name() const { return FILES_ENGINE_NAME; }

    void loadState(QSettings &settings);
    void saveState(QSettings &settings) const;
    void loadSettings(QSettings &settings);
    void saveSettings(QSettings &settings) const;

    Item *newItem(const QVariant & /* data */ = QVariant()) { return new Item; }
    Item *newItem(Engine::Item *item) { return new Item(qobject_cast<Item *> (item)); }

    Engine::SettingsWidget *newSettingsWidget(QWidget *parent = 0) { return new SettingsWidget(this,parent); }

  public:
    Core(const QString &tempStorageDir, QObject *parent = 0) : Engine::Core(tempStorageDir,parent) { }
    virtual ~Core() { }

    void swapRows(const int i, const int j);

    void addFile(const QString &fileName);
    void addFile(const QFileInfo &fileInfo);
    void addFiles(const QStringList &fileNameList);
    void addFiles(const QFileInfoList &fileInfoList);

  public slots:
    void init();
    void search(bool randomMode);
    void sizeLookup();
    void download();
    void infoCollect();
    void cancel();
  };
}

#endif
