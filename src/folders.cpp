/*
TRANSLATOR Folders::Item
*/
/*
TRANSLATOR Folders::LocalItem
*/
/*
TRANSLATOR Folders::RemoteItem
*/
/*
TRANSLATOR Folders::Core
*/
/*
TRANSLATOR Folders::DialogWidget
*/
/*
TRANSLATOR Folders::SettingsWidget
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

#include "exif.h"
#include "utils.h"
#include "engine.h"
#include "xtengine.h"
#include "folders.h"

using namespace Folders;

Item::Item(const QString &tempStorageDir) : XtEngine::Item()
{
  selectedIndex = -1;
  _tempStorageDir = tempStorageDir;
  connect(this,SIGNAL(fileListBuilt()),this,SLOT(fileListAvailable()));
}

void Item::buildRandomIndexesList()
{
  int i;

  toBeViewed.clear();
  for (i = 0; i < size(); toBeViewed << i++) ;
  for (i = 0; i < 2*size(); i++, toBeViewed.swap(qrand() % size(),qrand() % size())) ;
}

void Item::loadState(QSettings &settings, const QString & /* group */)
{
  selectedIndex = settings.value(LAST_INDEX,-1).toInt();
}

void Item::saveState(QSettings &settings, const QString &engineName, int id) const
{
  settings.beginGroup(QString("%1%2").arg(engineName).arg(id));

  settings.setValue(LAST_INDEX,selectedIndex);

  settings.endGroup();
}

void Item::extractFileName(bool randomMode)
{
  qDebug() << metaObject()->className() << "::extractFileName(" << randomMode << ")";

  _randomMode = randomMode;

  if (!size())
    buildFileList();
  else
    emit fileListBuilt();
}

void Item::fileListAvailable()
{
  qDebug() << metaObject()->className() << "::fileListAvailable()";

  if (size())
  {
    if (_randomMode)
    {
      if (!toBeViewed.size())
        buildRandomIndexesList();

      selectedIndex = toBeViewed.takeFirst();
    }
    else
      selectedIndex = (selectedIndex + 1) % size();

    doExtractFileName();
  }
  else
    emit searchCompleted(QFileInfo());
}

LocalItem::LocalItem(const QString &tempStorageDir) : Item(tempStorageDir)
{
}

LocalItem::LocalItem(LocalItem *item) : Item(item->tempStorageDir()), QDir(*item)
{
  _recursion = item->isRecursion();
}

void LocalItem::buildFileList()
{
  QFileInfo fileInfo;
  QDirIterator scan(absolutePath(),
                    ((isRecursion())? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags) |
                    QDirIterator::FollowSymlinks);

  qDebug() << metaObject()->className() << "::buildFileList()";

  fileList.clear();

  while (scan.hasNext())
    if ((fileInfo = scan.next()).isReadable() && fileInfo.isFile())
    {
      fileList << fileInfo.absoluteFilePath();

      qDebug() << "\tfound (file):" << fileInfo.absoluteFilePath().toAscii().constData();
    }

  qSort(fileList.begin(),fileList.end(),fileNameLessThan);

  emit fileListBuilt();
}

QVariant LocalItem::data() const
{
  QString str = QString("<font size=\"+1\">") + QDir::toNativeSeparators(absolutePath()) + "</font>";

  if (isRecursion())
    str += "<br>&nbsp;&nbsp;<i>" + tr("(with subfolders)") + "</i>";

  return str;
}

void LocalItem::doExtractFileName()
{
  emit searchCompleted(fileList.at(selectedIndex));
}

void LocalItem::saveSettings(QSettings &settings, const QString &engineName, int id) const
{
  settings.beginGroup(QString("%1%2").arg(engineName).arg(id));

  settings.setValue(FOLDER,QDir::toNativeSeparators(absolutePath()));
  settings.setValue(SUBFOLDERS,isRecursion());
  settings.setValue(REMOTE,false);

  settings.endGroup();
}

bool LocalItem::fileNameLessThan(const QFileInfo &f1, const QFileInfo &f2)
{
  return (f1.absoluteFilePath().compare(f2.absoluteFilePath()) < 0);
}

RemoteItem::RemoteItem(const QString &tempStorageDir) : Item(tempStorageDir)
{
  isDownloading = isListing = false;
  _passiveTransfer = true;
  ftpClient = new QFtp(this);

  connect(ftpClient,SIGNAL(done(bool)),this,SLOT(ftpDone(bool)));
  connect(ftpClient,SIGNAL(listInfo(const QUrlInfo &)),
          this,SLOT(ftpListInfo(const QUrlInfo &)));
}

RemoteItem::RemoteItem(RemoteItem *item) : Item(item->tempStorageDir()), QUrl(*item)
{
  isDownloading = isListing = false;
  _passiveTransfer = item->_passiveTransfer;
  ftpClient = new QFtp(this);

  connect(ftpClient,SIGNAL(done(bool)),this,SLOT(ftpDone(bool)));
  connect(ftpClient,SIGNAL(listInfo(const QUrlInfo &)),
          this,SLOT(ftpListInfo(const QUrlInfo &)));
}

void RemoteItem::buildFileList()
{
  if (isValidItem())
  {
    qDebug() << metaObject()->className() << "::buildFileList()";

    isListing = true;
    fileList.clear();
    ftpClient->connectToHost(host());
    ftpClient->login(userName(),password());
    ftpClient->list(path());
    ftpClient->close();
  }
}

void RemoteItem::ftpDone(bool error)
{
  if (!ftpClient->hasPendingCommands())
  {
    if (isDownloading)
    {
      isDownloading = false;
      ftpFile.close();

      if (error)
        emit searchCompleted(QFileInfo());
      else
        emit searchCompleted(ftpFile.fileName());
    }
    else if (isListing)
    {
      qDebug() << "\tlisting completed";

      isListing = false;
      emit fileListBuilt();
    }
  }
}

void RemoteItem::ftpListInfo(const QUrlInfo &listInfo)
{
  if (listInfo.isFile())
  {
    qDebug() << "\tfound (file): " << listInfo.name().toAscii().constData();

    fileList << listInfo;
  }
}

QVariant RemoteItem::data() const
{
  QUrl url = *this;

  url.setPassword("****");

  return QString("<font size=\"+1\">") + url.toString() + "</font><br>" +
         QString("<i>") + tr("Transfer mode:") + " " +
         ((_passiveTransfer)? tr("passive") : tr("active")) + "</i>";
}

void RemoteItem::doExtractFileName()
{
  QString fileName = fileList.at(selectedIndex).name();

  isDownloading = true;
  ftpFile.setFileName(tempStorageDir() + "/" + fileName);
  ftpFile.open(QIODevice::WriteOnly);

  ftpClient->connectToHost(host());
  ftpClient->login(userName(),password());
  ftpClient->cd(path());
  ftpClient->get(fileName,&ftpFile);
  ftpClient->close();
}

void RemoteItem::saveSettings(QSettings &settings, const QString &engineName, int id) const
{
  QUrl url = *this;
  settings.beginGroup(QString("%1%2").arg(engineName).arg(id));

  url.setPassword(QString(url.password().toAscii().toBase64()));

  settings.setValue(FOLDER,url.toString());
  settings.setValue(PASSIVE_TRANSFER,isTransferPassive());
  settings.setValue(REMOTE,true);

  settings.endGroup();
}

void Core::init()
{
  currentFileName.clear();

  emit initCompleted();
}

void Core::search(bool randomMode)
{
  qDebug() << name().toAscii().constData() << "::Core::search(" << randomMode << ")";

  if (!list.size())
    emit searchCompleted(false);
  else
  {
    currentItemIndex = (currentItemIndex + 1) % list.size();
    Item *item = qobject_cast<Item *> (list.at(currentItemIndex));
    item->extractFileName(randomMode);
  }
}

void Core::fileSearchCompleted(const QFileInfo &fileName)
{
  qDebug() << name().toAscii().constData()
           << "::fileSearchCompleted(" << fileName.absoluteFilePath().toAscii().constData() << ")";

  if (fileName.isFile())
  {
    currentFileName = fileName.absoluteFilePath();
    emit searchCompleted(QImageReader(currentFileName).canRead());
  }
  else
    emit searchCompleted(false);
}

void Core::sizeLookup()
{
  QSize size = QImageReader(currentFileName).size();

  qDebug() << name().toAscii().constData() << "::Core::sizeLookup()";

  emit sizeLookupCompleted(size);
}

void Core::download()
{
  qDebug() << name().toAscii().constData() << "::Core::download()";

  emit downloadCompleted(true,currentFileName);
}

void Core::infoCollect()
{
  PhotoInfo photoInfo;
  Exif::Tags tags(currentFileName);
  Item *item = qobject_cast<Item *> (list.at(currentItemIndex));

  qDebug() << name().toAscii().constData() << "::Core::infoCollect()";

  photoInfo.owner = tags.owner();
  photoInfo.title = tags.title();
  photoInfo.description = tags.description();
  if (item->isRemote())
    photoInfo.sourceUrl = *dynamic_cast<QUrl *> (list.at(currentItemIndex));
  else
    photoInfo.sourceUrl = QUrl::fromLocalFile(dynamic_cast<QDir *> (list.at(currentItemIndex))->absolutePath());

  emit infoCollectCompleted(photoInfo);
}

void Core::cancel()
{
  qDebug() << name().toAscii().constData() << "::Core::cancel()";

  emit cancelCompleted();
}

void Core::loadSettings(QSettings &settings)
{
  Item *item;
  QStringListIterator group(settings.childGroups());

  qDebug() << name().toAscii().constData() << "::Core::loadSettings(...)";

  setActive(settings.value(ENGINE_ACTIVE,true).toBool());

  while (group.hasNext())
  {
    settings.beginGroup(group.peekNext());

    item = qobject_cast<Item *> (newItem(settings.value(REMOTE,false)));
    if (settings.value(REMOTE,false).toBool())
    {
      QUrl url = settings.value(FOLDER,QString()).toString();

      url.setPassword(QString(QByteArray::fromBase64(url.password().toAscii())));
      qobject_cast<RemoteItem *> (item)->setUrl(url.toString());
      qobject_cast<RemoteItem *> (item)->setPassiveTransfer(settings.value(PASSIVE_TRANSFER,true).toBool());
    }
    else
    {
      qobject_cast<LocalItem *> (item)->setPath(settings.value(FOLDER,QString()).toString());
      qobject_cast<LocalItem *> (item)->setRecursion(settings.value(SUBFOLDERS,false).toBool());
    }

    if (item->isRemote() || qobject_cast<LocalItem *> (item)->isReadable())
    {
      item->loadState(settings,group.peekNext());

      list << item;
    }
    else
      delete item;

    settings.endGroup();

    group.next();
  }

  submit();
}

Engine::Item *Core::newItem(const QVariant &data)
{
  Item *newItem;

  if (data.toBool())
    newItem = new RemoteItem(tempStorageDir());
  else
    newItem = new LocalItem(tempStorageDir());

  connect(newItem,SIGNAL(searchCompleted(const QFileInfo &)),
          this,SLOT(fileSearchCompleted(const QFileInfo &)));

  return newItem;
}

Engine::Item *Core::newItem(Engine::Item *item)
{
  Item *newItem;

  if (qobject_cast<Item *> (item)->isRemote())
    newItem = new RemoteItem(qobject_cast<RemoteItem *> (item));
  else
    newItem = new LocalItem(qobject_cast<LocalItem *> (item));

  connect(newItem,SIGNAL(searchCompleted(const QFileInfo &)),
          this,SLOT(fileSearchCompleted(const QFileInfo &)));

  return newItem;
}

DialogWidget::DialogWidget(XtEngine::Core *core, QWidget *parent) : XtEngine::DialogWidget(core,parent)
{
  QCompleter *completer = new QCompleter(this);
  QVBoxLayout *mainLayout = new QVBoxLayout;
  QHBoxLayout *radioLayout = new QHBoxLayout;
  QDirModel *dirModel = new QDirModel(completer);

  dirModel->setFilter(QDir::AllDirs | QDir::Drives | QDir::NoDotAndDotDot);

  rbLocal = new QRadioButton(tr("Local"),this);
  rbRemote = new QRadioButton(tr("Remote"),this);
  radioLayout->addWidget(rbLocal);
  radioLayout->addWidget(rbRemote);

  QHBoxLayout *topLocalLayout = new QHBoxLayout;
  QVBoxLayout *mainLocalLayout = new QVBoxLayout;

  QGroupBox *localGroupBox = new QGroupBox(this);
  localGroupBox->setTitle(tr("Local"));
  localGroupBox->setEnabled(false);
  leLocalPath = new QLineEdit(this);
  completer->setModel(dirModel);
  leLocalPath->setCompleter(completer);
  leLocalPath->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Maximum);
  cbIncludeSubfolders = new QCheckBox(this);
  cbIncludeSubfolders->setText(tr("Include subfolders"));

  QToolButton *tbFolderDialog = new QToolButton(this);
  tbFolderDialog->setIcon(QIcon(":/images/folder"));
  connect(tbFolderDialog,SIGNAL(clicked()),this,SLOT(chooseFolder()));

  topLocalLayout->addWidget(new QLabel(tr("Folder:"),this));
  topLocalLayout->addWidget(leLocalPath);
  topLocalLayout->addWidget(tbFolderDialog);

  mainLocalLayout->addLayout(topLocalLayout);
  mainLocalLayout->addWidget(cbIncludeSubfolders);

  localGroupBox->setLayout(mainLocalLayout);

  QGroupBox *remoteGroupBox = new QGroupBox(this);
  remoteGroupBox->setTitle(tr("Remote"));
  remoteGroupBox->setEnabled(false);
  leServer = new QLineEdit(this);
  leServer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Maximum);
  sbPort = new QSpinBox(this);
  sbPort->setMinimum(1);
  sbPort->setMaximum(65535);
  sbPort->setValue(21);
  cbTransfer = new QComboBox(this);
  cbTransfer->addItem(tr("Passive"),QFtp::Passive);
  cbTransfer->addItem(tr("Active"),QFtp::Active);
  cbTransfer->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cbTransfer->setCurrentIndex(0);
  leUserName = new QLineEdit(this);
  leUserName->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Maximum);
  lePassword = new QLineEdit(this);
  lePassword->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Maximum);
  lePassword->setEchoMode(QLineEdit::Password);
  leRemotePath = new QLineEdit(this);
  leRemotePath->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Maximum);

  QGridLayout *mainRemoteLayout = new QGridLayout;
  mainRemoteLayout->addWidget(new QLabel(tr("Server:"),this),0,0);
  mainRemoteLayout->addWidget(leServer,0,1);
  mainRemoteLayout->addWidget(new QLabel(tr("Port:"),this),0,2);
  mainRemoteLayout->addWidget(sbPort,0,3);
  mainRemoteLayout->addWidget(new QLabel(tr("Transfer mode:"),this),0,4);
  mainRemoteLayout->addWidget(cbTransfer,0,5);
  mainRemoteLayout->addWidget(new QLabel(tr("Username:"),this),1,0);
  mainRemoteLayout->addWidget(leUserName,1,1);
  mainRemoteLayout->addWidget(new QLabel(tr("Password:"),this),1,3);
  mainRemoteLayout->addWidget(lePassword,1,4,1,2);
  mainRemoteLayout->addWidget(new QLabel(tr("Path:"),this),2,0);
  mainRemoteLayout->addWidget(leRemotePath,2,1,1,5);

  remoteGroupBox->setLayout(mainRemoteLayout);

  mainLayout->addLayout(radioLayout);
  mainLayout->addWidget(localGroupBox);
  mainLayout->addWidget(remoteGroupBox);

  connect(rbLocal,SIGNAL(toggled(bool)),localGroupBox,SLOT(setEnabled(bool)));
  connect(rbLocal,SIGNAL(clicked()),leLocalPath,SLOT(setFocus()));
  connect(rbRemote,SIGNAL(toggled(bool)),remoteGroupBox,SLOT(setEnabled(bool)));
  connect(rbRemote,SIGNAL(clicked()),leServer,SLOT(setFocus()));

  setLayout(mainLayout);

  rbLocal->setChecked(true);
  leLocalPath->setFocus();
}

void DialogWidget::chooseFolder()
{
#ifdef Q_WS_X11
  QString dirName = QFileDialog::getExistingDirectory(this,tr("Select a folder"),QDir::homePath(),
                                                      QFileDialog::ShowDirsOnly |
                                                      QFileDialog::DontResolveSymlinks |
                                                      QFileDialog::DontUseNativeDialog);
#else
  QString dirName = QFileDialog::getExistingDirectory(this,tr("Select a folder"),
                                                      QDesktopServices::storageLocation(QDesktopServices::PicturesLocation),
                                                      QFileDialog::ShowDirsOnly |
                                                      QFileDialog::DontResolveSymlinks |
                                                      QFileDialog::DontUseNativeDialog);
#endif

  if (!dirName.isEmpty())
    leLocalPath->setText(QDir::toNativeSeparators(dirName));
}

XtEngine::Item *DialogWidget::returnItem()
{
  XtEngine::Item *item = qobject_cast<XtEngine::Item *> (core()->newItem(!rbLocal->isChecked()));

  if (rbLocal->isChecked())
  {
    LocalItem *localItem = qobject_cast<LocalItem *> (item);

    localItem->setPath(leLocalPath->text());
    localItem->setRecursion(cbIncludeSubfolders->isChecked());
  }
  else
  {
    RemoteItem *remoteItem = qobject_cast<RemoteItem *> (item);

    if (!leServer->text().isEmpty())
    {
      remoteItem->setScheme("ftp");
      remoteItem->setHost(leServer->text());
      remoteItem->setPort(sbPort->value());
      remoteItem->setUserName(leUserName->text());
      remoteItem->setPassword(lePassword->text());
      remoteItem->setPath(leRemotePath->text());
      remoteItem->setPassiveTransfer(static_cast<QFtp::TransferMode>
                                      (cbTransfer->itemData(cbTransfer->currentIndex()).toInt()) ==
                                       QFtp::Passive);
    }
  }

  return item;
}

void DialogWidget::setupFromItem(XtEngine::Item *item)
{
  if (item)
  {
    Item *folderItem = qobject_cast<Item *> (item);

    setWindowTitle(tr("Edit folder"));

    rbLocal->setChecked(!folderItem->isRemote());
    rbRemote->setChecked(folderItem->isRemote());

    if (folderItem->isRemote())
    {
      RemoteItem *remoteItem = qobject_cast<RemoteItem *> (item);

      leServer->setText(remoteItem->host());
      sbPort->setValue(remoteItem->port());
      cbTransfer->setCurrentIndex(cbTransfer->findData((remoteItem->isTransferPassive())?
                                                       QFtp::Passive : QFtp::Active));
      leUserName->setText(remoteItem->userName());
      lePassword->setText(remoteItem->password());
      leRemotePath->setText(remoteItem->path());
    }
    else
    {
      LocalItem *localItem = qobject_cast<LocalItem *> (item);

      leLocalPath->setText(QDir::toNativeSeparators(localItem->absolutePath()));
      cbIncludeSubfolders->setChecked(localItem->isRecursion());
    }
  }
  else
    setWindowTitle(tr("Add folder"));
}

SettingsWidget::SettingsWidget(Engine::Core *core, QWidget *parent) :
  XtEngine::SettingsWidget(core,parent), isRefreshing(false)
{
  setupUi(this);

  lvFolders->setModel(core);
  lvFolders->setItemDelegate(new XtEngine::ItemDelegate(this));
  lvFolders->setSelectionModel(core->selectionModel());
  connect(core->selectionModel(),SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this,SLOT(updateWidgets(QItemSelection,QItemSelection)));
}

void SettingsWidget::on_tbAdd_clicked(bool)
{
  XtEngine::Item *item;

  if ((item = newDialog()->execute()) && item->isValidItem())
  {
    *qobject_cast<Core *> (core()) << item;
    emit settingsModified();
  }
}

void SettingsWidget::on_tbDel_clicked(bool)
{
  core()->removeRows(lvFolders->selectionModel());
  emit settingsModified();
}

void SettingsWidget::on_lvFolders_doubleClicked(const QModelIndex &index)
{
  XtEngine::Item *item;
  Engine::Item *newItem;
  QVariant var;

  if (index.isValid())
  {
    item = qobject_cast<XtEngine::Item *> (core()->data(index,Qt::UserRole).value<Engine::Item *>());

    if ((newItem = newDialog()->execute(item)) && qobject_cast<XtEngine::Item *> (newItem)->isValidItem())
    {
      var.setValue(newItem);
      core()->setData(index,var,Qt::UserRole);
      emit settingsModified();
    }
  }
}

void SettingsWidget::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
}

void SettingsWidget::dropEvent(QDropEvent *event)
{
  QList<QUrl> urls = event->mimeData()->urls();
  QListIterator<QUrl> url(urls);
  LocalItem *item;

  while (url.hasNext())
  {
    item = qobject_cast<LocalItem *> (core()->newItem(QVariant(false)));
    item->setPath(url.next().toLocalFile());

    *qobject_cast<Core *> (core()) << item;
  }

  if (urls.size())
    emit settingsModified();
}

void SettingsWidget::updateWidgets(const QItemSelection & /* selected */,
                                   const QItemSelection & /* deselected */)
{
  tbDel->setEnabled(lvFolders->selectionModel()->selectedRows().size());
}
