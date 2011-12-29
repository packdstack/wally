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

#ifndef FOLDERS_H
#define FOLDERS_H

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

#include "ui_folders.h"
#include "engine.h"
#include "xtengine.h"

#define FOLDERS_ENGINE_NAME "Folders"

#define FOLDER "folder"
#define SUBFOLDERS "subfolders"
#define PASSIVE_TRANSFER "passiveTransfer"
#define REMOTE "remote"
#define LAST_INDEX "lastIndex"

namespace Folders
{
  class Item : public XtEngine::Item
  {
    Q_OBJECT

    bool _randomMode;
    QList<int> toBeViewed;
    QString _tempStorageDir;

    void saveState(QSettings &settings, const QString &engineName, int id) const;
    void loadSettings(QSettings & /* settings */, const QString & /* group */) { }

  protected:
    int selectedIndex;
    void buildRandomIndexesList();
    void clearRandomIndexesList() { toBeViewed.clear(); }

    virtual void buildFileList() = 0;
    virtual int size() const = 0;
    virtual void doExtractFileName() = 0;

  public:
    Item(const QString &tempStorageDir);
    virtual ~Item() { }

    virtual bool isRemote() const = 0;

    QString tempStorageDir() const { return _tempStorageDir; }

    void extractFileName(bool randomMode);

    void loadState(QSettings &settings, const QString &group);

  signals:
    void searchCompleted(const QFileInfo &fileName);
    void fileListBuilt();

  public slots:
    void fileListAvailable();
  };

  class LocalItem : public Item, public QDir
  {
    Q_OBJECT

    bool _recursion;
    QFileInfoList fileList;

    QVariant data() const;

    void buildFileList();
    int size() const { return fileList.size(); }

    void doExtractFileName();
    void saveSettings(QSettings &settings, const QString &engineName, int id) const;

    static bool fileNameLessThan(const QFileInfo &f1, const QFileInfo &f2);

    bool isRemote() const { return false; }
    bool isValidItem() const { return exists(); }

  public:
    LocalItem(const QString &tempStorageDir);
    LocalItem(LocalItem *item);
    virtual ~LocalItem() { }

    bool isRecursion() const { return _recursion; }

    void setRecursion(bool recursion) { _recursion = recursion; }

    bool equalTo(Engine::Item *item) const { return *this == *qobject_cast<LocalItem *> (item); }
  };

  class RemoteItem : public Item, public QUrl
  {
    Q_OBJECT

    QFile ftpFile;
    bool isDownloading;
    bool isListing;
    bool _passiveTransfer;
    QList<QUrlInfo> fileList;
    QFtp *ftpClient;

    QVariant data() const;

    void buildFileList();
    int size() const { return fileList.size(); }

    void saveSettings(QSettings &settings, const QString &engineName, int id) const;

    void doExtractFileName();

    bool isRemote() const { return true; }
    bool isValidItem() const { return (!isEmpty() && isValid()); }

  private slots:
    void ftpDone(bool error);
    void ftpListInfo(const QUrlInfo &listInfo);

  public:
    RemoteItem(const QString &tempStorageDir);
    RemoteItem(RemoteItem *item);
    virtual ~RemoteItem() { }

    void setPassiveTransfer(bool passiveTransfer) { _passiveTransfer = passiveTransfer; }

    bool isTransferPassive() const { return _passiveTransfer; }

    bool equalTo(Engine::Item *item) const { return *this == *qobject_cast<RemoteItem *> (item); }
  };

  class DialogWidget : public XtEngine::DialogWidget
  {
    Q_OBJECT

    QRadioButton *rbLocal;
    QRadioButton *rbRemote;

    QLineEdit *leLocalPath;
    QCheckBox *cbIncludeSubfolders;

    QLineEdit *leServer;
    QSpinBox *sbPort;
    QComboBox *cbTransfer;
    QLineEdit *leUserName;
    QLineEdit *lePassword;
    QLineEdit *leRemotePath;

    XtEngine::Item *returnItem();
    void setupFromItem(XtEngine::Item *item = 0);

  private slots:
    void chooseFolder();

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

  class SettingsWidget : public XtEngine::SettingsWidget, private Ui::FoldersSettingsWidget
  {
    Q_OBJECT

    bool isRefreshing;

    QPixmap buttonPixmap() const { return QPixmap(); }
    QIcon buttonIcon() const { return QIcon(":/images/folders"); }
    QString buttonText() const { return tr("Folders"); }

    XtEngine::Dialog *newDialog() { return new Dialog(qobject_cast<XtEngine::Core *> (core()),this); }

  private slots:
    void on_tbAdd_clicked(bool);
    void on_tbDel_clicked(bool);
    void on_lvFolders_doubleClicked(const QModelIndex &index);
    void updateWidgets(const QItemSelection &selected, const QItemSelection &deselected);

  protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

  public:
    SettingsWidget(Engine::Core *core, QWidget *parent = 0);
    virtual ~SettingsWidget() { }
  };

  class Core : public XtEngine::Core
  {
    Q_OBJECT

    QString currentFileName;

    QString name() const { return FOLDERS_ENGINE_NAME; }

    Engine::SettingsWidget *newSettingsWidget(QWidget *parent = 0) { return new SettingsWidget(this,parent); }

    Engine::Item *newItem(const QVariant &data = QVariant());
    Engine::Item *newItem(Engine::Item *item);

    void loadSettings(QSettings &settings);

  private slots:
    void fileSearchCompleted(const QFileInfo &fileName);

  public:
    Core(const QString &tempStorageDir, QObject *parent = 0) : XtEngine::Core(tempStorageDir,parent) { }
    virtual ~Core() { }

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
