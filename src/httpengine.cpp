/*
TRANSLATOR HttpEngine::Item
*/
/*
TRANSLATOR HttpEngine::Core
*/
/*
TRANSLATOR HttpEngine::SettingsWidget
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

#include <QtDebug>

#include "diskinfo.h"
#include "engine.h"
#include "httpengine.h"

using namespace HttpEngine;

Item::Item()
{
  _savePhotos = false;
  _pageIndex = _pages = _consecutiveSearchesFailed = 0;
}

Item::Item(Item *item)
{
  _pageIndex = item->_pageIndex;
  _pages = item->_pages;
  _savePhotos = item->_savePhotos;
  _photosPath = item->_photosPath;
  _consecutiveSearchesFailed = item->_consecutiveSearchesFailed;
}

void Item::loadState(QSettings &settings, const QString &group)
{
  settings.beginGroup(group);

  _pageIndex = settings.value(PAGE_INDEX,0).toInt();
  _pages = settings.value(PAGES,0).toInt();

  doLoadState(settings);

  settings.endGroup();
}

void Item::saveState(QSettings &settings, const QString &engineName, int id) const
{
  settings.beginGroup(QString("%1%2").arg(engineName).arg(id));

  settings.setValue(QString(PAGE_INDEX),QString::number(_pageIndex));
  settings.setValue(QString(PAGES),QString::number(_pages));

  doSaveState(settings);

  settings.endGroup();
}

void Item::loadSettings(QSettings &settings, const QString &group)
{
  settings.beginGroup(group);

  _savePhotos = settings.value("savePhotos",false).toBool();
  _photosPath = settings.value("photosPath",QString()).toString();

  doLoadSettings(settings);

  settings.endGroup();
}

void Item::saveSettings(QSettings &settings, const QString &engineName, int id) const
{
  settings.beginGroup(QString("%1%2").arg(engineName).arg(id));

  settings.setValue("savePhotos",_savePhotos);
  settings.setValue("photosPath",QDir::toNativeSeparators(_photosPath));

  doSaveSettings(settings);

  settings.endGroup();
}

QUrl Item::prepareInit(Operation &op)
{
  qDebug() << metaObject()->className() << "::prepareInit()";

  return doPrepareInit(op);
}

void Item::processInitResult(const QByteArray &response)
{
  QString debugMsg = QString(metaObject()->className()) + "::processInitResult() ->";

  if (response.size())
    debugMsg += QString("\n") + response.constData();
  else
    debugMsg += " empty";

  qDebug() << debugMsg;

  doProcessInitResult(response);
}

QUrl Item::prepareSearch(bool randomMode, Operation &op)
{
  QString debugMsg = QString(metaObject()->className()) + "::doCalculateNextIndex(" + ((randomMode)? "true" : "false") +
                     "," + QString::number(_pageIndex) + "," + QString::number(_pages) + ") ->";

  qDebug() << metaObject()->className() << "::prepareSearch(" << randomMode << ")";

  if (_pages && (_pageIndex >= _pages))
    resetPages();

  _pageIndex = doCalculateNextIndex(randomMode,_pageIndex,_pages);

  qDebug() << debugMsg << _pageIndex;

  return doPrepareSearch(op);
}

bool Item::processSearchResult(const QByteArray &response)
{
  QString debugMsg = QString(metaObject()->className()) + "::processSearchResult() ->";

  if (response.size())
    debugMsg += QString("\n") + response.constData();
  else
    debugMsg += " empty";

  qDebug() << debugMsg;

  if (doProcessSearchResult(response,_pages))
  {
    _consecutiveSearchesFailed = 0;
    return true;
  }
  else
  {
    ++_consecutiveSearchesFailed;
    if (_consecutiveSearchesFailed > 20)
      resetPages();
    return false;
  }
}

QSize Item::processSizeRequestResult(const QByteArray &response)
{
  QString debugMsg = QString(metaObject()->className()) + "::processSizeRequestResult() ->";

  if (response.size())
    debugMsg += QString("\n") + response.constData();
  else
    debugMsg += " empty";

  qDebug() << debugMsg;

  return doProcessSizeRequestResult(response);
}

void Item::processDownloadResult(const QFileInfo &fileInfo, bool belowThreshold)
{
  qDebug() << metaObject()->className()
           << "::processDownloadResult(" << fileInfo.absoluteFilePath().toAscii().constData() << ")";

  if (_savePhotos && !_photosPath.isEmpty())
  {
    if (!belowThreshold)
    {
      QFile::copy(fileInfo.absoluteFilePath(),QFileInfo(_photosPath,
                                                        fileInfo.fileName()).absoluteFilePath());

      qDebug() << "\tfile copied in" << _photosPath.toAscii().constData()
               << "\n\tfree disk space:" << (DiskInfo::freeSpace(fileInfo.absoluteFilePath()) / 1024) << "kbytes";
    }
    else
      qDebug() << "\tcannot copy file, free disk space:" << (DiskInfo::freeSpace(fileInfo.absoluteFilePath()) / 1024) << "kbytes";
  }

  doProcessDownloadResult(fileInfo);
}

PhotoInfo Item::processInfoCollectResult(const QByteArray &response)
{
  QString debugMsg = QString(metaObject()->className()) + "::processInfoCollectResult() ->";

  if (response.size())
    debugMsg += QString("\n") + response.constData();
  else
    debugMsg += " empty";

  qDebug() << debugMsg;

  return doProcessInfoCollectResult(response);
}

Core::Core(const QString &tempStorageDir, QObject *parent) : XtEngine::Core(tempStorageDir,parent),
  httpProxy(QNetworkProxy::NoProxy)
{
  canceling = cancelSignaled = false;

  initHttp = new Network::Manager(this);
  searchHttp = new Network::Manager(this);
  sizeRequestHttp = new Network::Manager(this);
  downloadHttp = new Network::Manager(this);
  infoCollectHttp = new Network::Manager(this);
}

void Core::setProxy(const QNetworkProxy &proxy)
{
  qDebug() << name().toAscii().constData() << "::Core::setProxy(...)";

  httpProxy = proxy;
}

QNetworkProxy Core::guessProxy(const QUrl &url)
{
  QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery(QNetworkProxyQuery(url));

  if (proxies.size())
  {
    QNetworkProxy localProxy = proxies.at(0);
    localProxy.setUser(httpProxy.user());
    localProxy.setPassword(httpProxy.password());

    qDebug() << name().toAscii().constData() << "::guessProxy()\n"
             << "\tusing proxy "
             << localProxy.user().toAscii().constData() << "@"
             << localProxy.hostName().toAscii().constData() << ":"
             << QString::number(localProxy.port()).toAscii().constData() << "\n"
             << "\tfor " << url.toString().toAscii().constData();

    return localProxy;
  }
  else
    return QNetworkProxy(QNetworkProxy::NoProxy);
}

void Core::init()
{
  qDebug() << name().toAscii().constData() << "::Core::init()";

  canceling = cancelSignaled = false;

  if (list.size())
  {
    Operation op = Get;
    currentItemIndex = (currentItemIndex + 1) % list.size();
    QUrl url(qobject_cast<Item *> (list.at(currentItemIndex))->prepareInit(op));

    qDebug() << "\tURL: " << (!(url.isEmpty())? url.toString().toAscii().constData() : "empty");

    if (url.isEmpty())
      emit initCompleted();
    else
    {
      if (!httpProxy.hostName().compare("0.0.0.0"))
        initHttp->setProxy(guessProxy(url));
      else
        initHttp->setProxy(httpProxy);

      switch (op)
      {
        case Get:
          currentReply = initHttp->get(url);
          break;

        case Post:
          currentReply = initHttp->post(url);
          break;

        default:
          emit initCompleted();
          return;
      }

      connect(currentReply.data(),SIGNAL(finished(bool,const QByteArray &)),this,SLOT(httpInitDone(bool,const QByteArray &)));
    }
  }
  else
    emit initCompleted();
}

void Core::search(bool randomMode)
{
  qDebug() << name().toAscii().constData() << "::Core::search(" << randomMode << ")";

  if (list.size())
  {
    Operation op = Get;
    QUrl url(qobject_cast<Item *> (list.at(currentItemIndex))->prepareSearch(randomMode,op));

    qDebug() << "\tURL:" << (!(url.isEmpty())? url.toString().toAscii().constData() : "empty");

    if (url.isEmpty())
      emit searchCompleted(false);
    else if (url.isRelative())
      emit searchCompleted(true);
    else
    {
      if (!httpProxy.hostName().compare("0.0.0.0"))
        searchHttp->setProxy(guessProxy(url));
      else
        searchHttp->setProxy(httpProxy);

      switch (op)
      {
        case Get:
          currentReply = searchHttp->get(url);
          break;

        case Post:
          currentReply = searchHttp->post(url);
          break;

        default:
          emit searchCompleted(false);
          return;
      }

      connect(currentReply.data(),SIGNAL(finished(bool,const QByteArray &)),this,SLOT(httpSearchDone(bool,const QByteArray &)));
    }
  }
  else
    emit searchCompleted(false);
}

void Core::sizeLookup()
{
  Operation op = Get;
  Item *item = qobject_cast<Item *> (list.at(currentItemIndex));
  QUrl url(item->prepareSizeRequest(op));

  qDebug() << name().toAscii().constData() << "::Core::sizeLookup()";
  qDebug() << "\tURL: " << (!(url.isEmpty())? url.toString().toAscii().constData() : "empty");

  if (url.isEmpty())
    emit sizeLookupCompleted(QSize(-1,-1));
  else if (!url.isRelative())
  {
    if (!httpProxy.hostName().compare("0.0.0.0"))
      sizeRequestHttp->setProxy(guessProxy(url));
    else
      sizeRequestHttp->setProxy(httpProxy);

      switch (op)
      {
        case Get:
          currentReply = sizeRequestHttp->get(url);
          break;

        case Post:
          currentReply = sizeRequestHttp->post(url);
          break;

        default:
          emit sizeLookupCompleted(QSize(-1,-1));
          return;
      }

    connect(currentReply.data(),SIGNAL(finished(bool,const QByteArray &)),this,SLOT(httpSizeRequestDone(bool,const QByteArray &)));
  }
  else
    emit sizeLookupCompleted(item->processSizeRequestResult(QByteArray()));
}

void Core::download()
{
  Operation op = Get;
  QUrl url(qobject_cast<Item *> (list.at(currentItemIndex))->prepareDownload(op));

  httpFile = tempStorageDir() + "/images/" + QFileInfo(url.path()).fileName();

  qDebug() << name().toAscii().constData() << "::Core::download()";
  qDebug() << "\tURL: "
           << (!(url.isEmpty())? url.toString().toAscii().constData() : "empty");

  if (url.isEmpty())
    emit downloadCompleted(false,QFileInfo());
  else
  {
    if (!httpProxy.hostName().compare("0.0.0.0"))
      downloadHttp->setProxy(guessProxy(url));
    else
      downloadHttp->setProxy(httpProxy);

    currentReply = downloadHttp->get(url,httpFile.absoluteFilePath());

    connect(currentReply.data(),SIGNAL(finished(bool,const QByteArray &)),this,SLOT(httpDownloadDone(bool,const QByteArray &)));
  }
}

void Core::infoCollect()
{
  Operation op = Get;
  Item *item = qobject_cast<Item *> (list.at(currentItemIndex));
  QUrl url(item->prepareInfoCollect(op));

  qDebug() << name().toAscii().constData() << "::Core::infoCollect()";
  qDebug() << "\tURL: " << (!(url.isEmpty())? url.toString().toAscii().constData() : "empty");

  if (url.isEmpty())
    emit infoCollectCompleted(PhotoInfo());
  else if (!url.isRelative())
  {
    if (!httpProxy.hostName().compare("0.0.0.0"))
      infoCollectHttp->setProxy(guessProxy(url));
    else
      infoCollectHttp->setProxy(httpProxy);

      switch (op)
      {
        case Get:
          currentReply = infoCollectHttp->get(url);
          break;

        case Post:
          currentReply = infoCollectHttp->post(url);
          break;

        default:
          emit infoCollectCompleted(PhotoInfo());
          return;
      }

    connect(currentReply.data(),SIGNAL(finished(bool,const QByteArray &)),this,SLOT(httpInfoCollectDone(bool,const QByteArray &)));
  }
  else
    emit infoCollectCompleted(item->processInfoCollectResult(QByteArray()));
}

void Core::cancel()
{
  qDebug() << name().toAscii().constData() << "::Core::cancel()";

  canceling = true;
  cancelSignaled = false;

  if (!currentReply.isNull())
    currentReply->abort();
}

void Core::httpInitDone(bool ok, const QByteArray &data)
{
  qDebug() << name().toAscii().constData() << "::Core::httpInitDone(" << ok << ")";

  if (ok && !data.isEmpty())
  {
    Item *item = qobject_cast<Item *> (list.at(currentItemIndex));
    item->processInitResult(data);
  }

  if (canceling)
  {
    if (!cancelSignaled)
    {
      cancelSignaled = true;
      emit cancelCompleted();
    }
  }
  else
    emit initCompleted();

  sender()->deleteLater();
}

void Core::httpSearchDone(bool ok, const QByteArray &data)
{
  qDebug() << name().toAscii().constData() << "::Core::httpSearchDone(" << ok << ")";

  if (canceling)
  {
    if (!cancelSignaled)
    {
      cancelSignaled = true;
      emit cancelCompleted();
    }
  }
  else if (ok)
  {
    if (!data.isEmpty())
    {
      Item *item = qobject_cast<Item *> (list.at(currentItemIndex));
      emit searchCompleted(item->processSearchResult(data));
    }
    else
      emit searchCompleted(false);
  }
  else
    emit searchCompleted(false);

  sender()->deleteLater();
}

void Core::httpSizeRequestDone(bool ok, const QByteArray &data)
{
  qDebug() << name().toAscii().constData() << "::Core::httpSizeRequestDone(" << ok << ")";

  if (canceling)
  {
    if (!cancelSignaled)
    {
      cancelSignaled = true;
      emit cancelCompleted();
    }
  }
  else if (ok)
  {
    if (!data.isEmpty())
    {
      Item *item = qobject_cast<Item *> (list.at(currentItemIndex));
      emit sizeLookupCompleted(item->processSizeRequestResult(data));
    }
    else
      emit sizeLookupCompleted(QSize(-1,-1));
  }
  else
    emit sizeLookupCompleted(QSize(-1,-1));

  sender()->deleteLater();
}

void Core::httpDownloadDone(bool ok, const QByteArray & /* data */)
{
  qDebug() << name().toAscii().constData() << "::Core::httpDownloadDone(" << ok << ")";

  if (canceling)
  {
    if (!cancelSignaled)
    {
      cancelSignaled = true;
      emit cancelCompleted();
    }
  }
  else if (ok)
  {
    Item *item = qobject_cast<Item *> (list.at(currentItemIndex));
    item->processDownloadResult(httpFile,
                                DiskInfo::freeSpace(httpFile.absoluteFilePath()) <=
                                (_diskSpaceThreshold + httpFile.size()));
    emit downloadCompleted(true,httpFile);
  }
  else
    emit downloadCompleted(false,QFileInfo());

  sender()->deleteLater();
}

void Core::httpInfoCollectDone(bool ok, const QByteArray &data)
{
  qDebug() << name().toAscii().constData() << "::Core::httpInfoCollectDone(" << ok << ")";

  if (canceling)
  {
    if (!cancelSignaled)
    {
      cancelSignaled = true;
      emit cancelCompleted();
    }
  }
  else if (ok)
  {
    if (!data.isEmpty())
    {
      Item *item = qobject_cast<Item *> (list.at(currentItemIndex));
      emit infoCollectCompleted(item->processInfoCollectResult(data));
    }
    else
      emit infoCollectCompleted(PhotoInfo());
  }
  else
    emit infoCollectCompleted(PhotoInfo());

  sender()->deleteLater();
}

SettingsWidget::SettingsWidget(Engine::Core *core, QWidget *parent)
  : XtEngine::SettingsWidget(core,parent), isRefreshing(false)
{
  QCompleter *completer = new QCompleter(this);
  QDirModel *dirModel = new QDirModel(completer);

  setupUi(this);

  dirModel->setFilter(QDir::AllDirs | QDir::Drives | QDir::NoDotAndDotDot);
  completer->setModel(dirModel);
  lePhotosPath->setCompleter(completer);

  lvSets->setModel(core);
  lvSets->setItemDelegate(new XtEngine::ItemDelegate(this));
  lvSets->setSelectionModel(core->selectionModel());
  connect(core->selectionModel(),SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this,SLOT(updateWidgets(QItemSelection,QItemSelection)));
}

void SettingsWidget::on_tbAdd_clicked()
{
  XtEngine::Item *item;

  if ((item = newDialog()->execute()) && item->isValidItem())
  {
    *core() << item;
    emit settingsModified();
  }
}

void SettingsWidget::on_tbDel_clicked()
{
  core()->removeRows(lvSets->selectionModel());
  emit settingsModified();
}

void SettingsWidget::on_lvSets_doubleClicked(const QModelIndex &index)
{
  QVariant var;
  XtEngine::Item *item;
  Engine::Item *newItem;

  if (index.isValid())
  {
    item = qobject_cast <XtEngine::Item *> (core()->data(index,Qt::UserRole).value<Engine::Item *>());

    if ((newItem = newDialog()->execute(item)) && qobject_cast<XtEngine::Item *> (newItem)->isValidItem())
    {
      Item *httpItem = qobject_cast<Item *> (item);

      qobject_cast<Item *> (newItem)->setPhotosPath(httpItem->photosPath());
      qobject_cast<Item *> (newItem)->setSavePhotos(httpItem->arePhotosSaved());
      var.setValue(newItem);
      core()->setData(index,var,Qt::UserRole);
      emit settingsModified();
    }
  }
}

void SettingsWidget::on_tbPhotosPath_clicked()
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
  {
    lePhotosPath->setText(QDir::toNativeSeparators(dirName));
    emit settingsModified();
  }
}

void SettingsWidget::updateWidgets(const QItemSelection & /* selected */,
                                   const QItemSelection & /* deselected */)
{
  gbOptions->setEnabled(lvSets->selectionModel()->selectedRows().size());

  if (lvSets->selectionModel()->selectedRows().size() == 1)
  {
    QModelIndex index = lvSets->selectionModel()->selectedRows().at(0);
    Item *item = qobject_cast<Item *> (core()->data(index,Qt::UserRole).value<Engine::Item *>());

    isRefreshing = true;
    cbSavePhotos->setTristate(false);
    cbSavePhotos->setChecked(item->arePhotosSaved());
    lePhotosPath->setText(QDir::toNativeSeparators(item->photosPath()));
    lePhotosPath->setEnabled(cbSavePhotos->isChecked());
    tbPhotosPath->setEnabled(cbSavePhotos->isChecked());
    isRefreshing = false;
  }
  else if (lvSets->selectionModel()->selectedRows().size() > 1)
  {
    int i;
    bool changed;
    QModelIndex firstIndex = lvSets->selectionModel()->selectedRows().at(0);
    Item *firstItem = qobject_cast<Item *> (core()->data(firstIndex,Qt::UserRole).value<Engine::Item *>());

    for (i = 1, changed = false; (i < lvSets->selectionModel()->selectedRows().size()) && (!changed); ++i)
    {
      QModelIndex index = lvSets->selectionModel()->selectedRows().at(i);
      Item *item = qobject_cast<Item *> (core()->data(index,Qt::UserRole).value<Engine::Item *>());

      changed = (item->arePhotosSaved() != firstItem->arePhotosSaved()) ||
                (QFileInfo(item->photosPath()) != QFileInfo(firstItem->photosPath()));
    }

    isRefreshing = true;
    if (changed)
    {
      cbSavePhotos->setTristate(true);
      cbSavePhotos->setCheckState(Qt::PartiallyChecked);
      lePhotosPath->clear();
      lePhotosPath->setEnabled(false);
      tbPhotosPath->setEnabled(false);
    }
    else
    {
      cbSavePhotos->setTristate(false);
      cbSavePhotos->setChecked(firstItem->arePhotosSaved());
      lePhotosPath->setText(QDir::toNativeSeparators(firstItem->photosPath()));
      lePhotosPath->setEnabled(cbSavePhotos->isChecked());
      tbPhotosPath->setEnabled(cbSavePhotos->isChecked());
    }
    isRefreshing = false;
  }

  tbDel->setEnabled(lvSets->selectionModel()->selectedRows().size());
}

void SettingsWidget::on_cbSavePhotos_stateChanged(int state)
{
  Item *item;

  if (!isRefreshing)
  {
    QListIterator<QModelIndex> index(lvSets->selectionModel()->selectedRows());

    while (index.hasNext())
    {
      item = qobject_cast<Item *> (core()->data(index.next(),Qt::UserRole).value<Engine::Item *>());
      item->setSavePhotos(state == Qt::Checked);
    }

    lePhotosPath->setEnabled(state == Qt::Checked);
    tbPhotosPath->setEnabled(state == Qt::Checked);

    emit settingsModified();
  }
}

void SettingsWidget::on_lePhotosPath_textChanged(const QString &text)
{
  Item *item;

  if (!isRefreshing)
  {
    QListIterator<QModelIndex> index(lvSets->selectionModel()->selectedRows());

    while (index.hasNext())
    {
      item = qobject_cast<Item *> (core()->data(index.next(),Qt::UserRole).value<Engine::Item *>());
      item->setPhotosPath(text);
    }

    emit settingsModified();
  }
}
