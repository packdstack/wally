/*
TRANSLATOR Files::Item
*/
/*
TRANSLATOR Files::Core
*/
/*
TRANSLATOR Files::LabelPreview
*/
/*
TRANSLATOR Files::SettingsWidget
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

#include "exif.h"
#include "utils.h"
#include "engine.h"
#include "files.h"

using namespace Files;

QVariant Item::data() const
{
  return fileName();
}

void Core::doSubmit()
{
  toBeViewed.clear();
}

void Core::swapRows(const int i, const int j)
{
  list.swap(i,j);
  reset();
}

void Core::buildRandomIndexesList()
{
  int i;

  for (i = 0; i < list.size(); toBeViewed << i++) ;
  for (i = 0; i < 2*list.size(); i++, toBeViewed.swap(qrand() % list.size(),qrand() % list.size())) ;
}

void Core::init()
{
  emit initCompleted();
}

void Core::search(bool randomMode)
{
  qDebug() << name().toAscii().constData() << "::Core::search(" << randomMode << ")";

  if (!list.size())
    emit searchCompleted(false);
  else
  {
    if (randomMode)
    {
      if (!toBeViewed.size())
        buildRandomIndexesList();

      selectedIndex = toBeViewed.takeFirst();
    }
    else
      selectedIndex = (selectedIndex + 1) % list.size();

    emit searchCompleted(true);
  }
}

void Core::sizeLookup()
{
  QSize size = QImageReader(qobject_cast<Item *> (list.at(selectedIndex))->absoluteFilePath()).size();

  qDebug() << name().toAscii().constData() << "::Core::sizeLookup()";

  emit sizeLookupCompleted(size);
}

void Core::download()
{
  qDebug() << name().toAscii().constData() << "::Core::download()";

  emit downloadCompleted(true,*qobject_cast<Item *> (list.at(selectedIndex)));
}

void Core::infoCollect()
{
  Item *item = qobject_cast<Item *> (list.at(selectedIndex));
  PhotoInfo photoInfo;
  Exif::Tags tags(item->absoluteFilePath());

  qDebug() << name().toAscii().constData() << "::Core::infoCollect()";

  photoInfo.owner = tags.owner();
  photoInfo.title = tags.title();
  photoInfo.description = tags.description();
  photoInfo.sourceUrl = QUrl::fromLocalFile(item->absoluteFilePath());

  emit infoCollectCompleted(photoInfo);
}

void Core::cancel()
{
  qDebug() << name().toAscii().constData() << "::Core::cancel()";

  emit cancelCompleted();
}

void Core::addFile(const QString &fileName)
{
  if (!fileName.isEmpty() && QImageReader(fileName).canRead())
  {
    *this << new Item(QFileInfo(fileName));
    reset();
  }
}

void Core::addFiles(const QStringList &fileNameList)
{
  int cnt = 0;
  QStringListIterator fileName(fileNameList);
  QProgressDialog dialog(tr("Loading folder..."),QString(),0,fileNameList.size());

  while (fileName.hasNext())
  {
    if (!fileName.peekNext().isEmpty() && QImageReader(fileName.peekNext()).canRead())
    {
      *this << new Item(QFileInfo(fileName.peekNext()));
      ++cnt;
    }

    dialog.setValue(dialog.value() + 1);

    fileName.next();
  }

  if (cnt)
    reset();
}

void Core::addFile(const QFileInfo &fileInfo)
{
  if (QImageReader(fileInfo.absoluteFilePath()).canRead())
  {
    *this << new Item(fileInfo);
    reset();
  }
}

void Core::addFiles(const QFileInfoList &fileInfoList)
{
  int cnt = 0;
  QListIterator<QFileInfo> fileInfo(fileInfoList);

  while (fileInfo.hasNext())
  {
    if (QImageReader(fileInfo.peekNext().absoluteFilePath()).canRead())
    {
      *this << new Item(fileInfo.peekNext());
      ++cnt;
    }

    fileInfo.next();
  }

  if (cnt)
    reset();
}

void Core::loadState(QSettings &settings)
{
  selectedIndex = settings.value(LAST_INDEX,-1).toInt();
}

void Core::saveState(QSettings &settings) const
{
  if (list.size() && (selectedIndex != -1))
    settings.setValue(LAST_INDEX,selectedIndex);
}

void Core::loadSettings(QSettings &settings)
{
  QStringListIterator key(settings.childKeys());

  setActive(settings.value(ENGINE_ACTIVE,true).toBool());

  while (key.hasNext())
  {
    if (key.peekNext().compare(LAST_INDEX,Qt::CaseInsensitive) &&
        QFileInfo(settings.value(key.peekNext(),QString()).toString()).isReadable())
      list << new Item(QFileInfo(settings.value(key.peekNext(),QString()).toString()));

    key.next();
  }

  submit();
}

void Core::saveSettings(QSettings &settings) const
{
  int i = 0;
  QListIterator<Engine::Item *> item(list);

  settings.setValue(ENGINE_ACTIVE,isActive());

  while (item.hasNext())
    settings.setValue(QString("%1%2").arg(name()).arg(i++),
                      QDir::toNativeSeparators(qobject_cast<Item *>
                                                (item.next())->absoluteFilePath()));
}

LabelPreview::LabelPreview(QWidget *parent, Qt::WindowFlags f) : QLabel(parent,f)
{
  QFont labelFont(font());

  labelFont.setPointSize(19);
  labelFont.setBold(true);

  setFixedSize(sizeHint());
  setWordWrap(true);
  setFont(labelFont);
  setAlignment(Qt::AlignCenter);
  setStyleSheet("background-color: rgb(0, 0, 0); color: rgb(255, 255, 255);");
  clear();
}

void LabelPreview::setPixmap(const QPixmap &pixmap)
{
  if (pixmap.isNull())
    clear();
  else
    QLabel::setPixmap(pixmap.scaled(size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
}

void LabelPreview::clear()
{
  QLabel::clear();
  setText(tr("No preview available"));
}

SettingsWidget::SettingsWidget(Engine::Core *core, QWidget *parent) : Engine::SettingsWidget(core,parent),
                                                                      dontChangePreview(false)
{
  setupUi(this);

  vlPreview->insertWidget(0,lblPreview = new LabelPreview(this));

  lvPhotos->setModel(this->core());
  lvPhotos->setSelectionModel(this->core()->selectionModel());
  connect(this->core()->selectionModel(),SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this,SLOT(updateWidgets(QItemSelection,QItemSelection)));
}

void SettingsWidget::on_tbAdd_clicked(bool)
{
  QFileDialog dialog(this);

  dialog.setWindowTitle(tr("Select a photo"));
#ifdef Q_WS_X11
  dialog.setDirectory(QDir::homePath());
#else
  dialog.setDirectory(QDesktopServices::storageLocation(QDesktopServices::PicturesLocation));
#endif
  dialog.setNameFilters(getImageFilters());
  dialog.setFileMode(QFileDialog::ExistingFiles);
  dialog.exec();

  qobject_cast<Core *> (core())->addFiles(dialog.selectedFiles());
}

void SettingsWidget::on_tbFolderAdd_clicked(bool)
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
    qobject_cast<Core *> (core())->addFiles(QDir(dirName).entryInfoList(QDir::Files | QDir::Readable));
    emit settingsModified();
  }
}

void SettingsWidget::on_tbDel_clicked(bool)
{
  core()->removeRows(lvPhotos->selectionModel());
  emit settingsModified();
}

void SettingsWidget::on_lvPhotos_doubleClicked(const QModelIndex &index)
{
  Item *item;

  QString fileName;
  QFileDialog dialog(this);
  QVariant var;

  if (index.isValid())
  {
    item = qobject_cast<Item *> (core()->data(index,Qt::UserRole).value<Engine::Item *>());

    dialog.setWindowTitle(tr("Select a photo"));
    dialog.setDirectory(item->absolutePath());
    dialog.setNameFilters(getImageFilters());
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.selectFile(item->absoluteFilePath());
    dialog.exec();

    fileName = dialog.selectedFiles().at(0);

    if (!fileName.isEmpty() && (QFileInfo(fileName) != *item) && QImageReader(fileName).canRead())
    {
      Engine::Item *varItem = new Item(QFileInfo(fileName));

      var.setValue(varItem);
      core()->setData(index,var,Qt::UserRole);
      lblPreview->setPixmap(QPixmap(qobject_cast<Item *> (varItem)->absoluteFilePath()));
      emit settingsModified();
    }
  }
}

void SettingsWidget::on_tbMoveUp_clicked(bool)
{
  int row = 0;

  if ((lvPhotos->selectionModel()->selectedRows().size() == 1) &&
      ((row = lvPhotos->selectionModel()->selectedRows().at(0).row())))
  {
    dontChangePreview = true;
    qobject_cast<Core *> (core())->swapRows(row,row-1);
    lvPhotos->setCurrentIndex(core()->index(row-1,0));
    dontChangePreview = false;
    emit settingsModified();
  }
}

void SettingsWidget::on_tbMoveDown_clicked(bool)
{
  int row = 0;

  if ((lvPhotos->selectionModel()->selectedRows().size() == 1) &&
      ((row = lvPhotos->selectionModel()->selectedRows().at(0).row()) < (core()->rowCount() - 1)))
  {
    dontChangePreview = true;
    qobject_cast<Core *> (core())->swapRows(row,row+1);
    lvPhotos->setCurrentIndex(core()->index(row+1,0));
    dontChangePreview = false;
    emit settingsModified();
  }
}

void SettingsWidget::updateWidgets(const QItemSelection & /* selected */,
                                   const QItemSelection & /* deselected */)
{
  Item *item;

  if (!dontChangePreview)
  {
    if (lvPhotos->selectionModel()->selectedRows().size() == 1)
    {
      QModelIndex index = core()->index(lvPhotos->selectionModel()->selectedRows().at(0).row(),0);
      item = qobject_cast<Item *> (core()->data(index,Qt::UserRole).value<Engine::Item *>());
      lblPreview->setPixmap(QPixmap(item->absoluteFilePath()));
    }
    else
      lblPreview->clear();
  }

  tbDel->setEnabled(lvPhotos->selectionModel()->selectedRows().size());
  tbMoveUp->setEnabled((lvPhotos->selectionModel()->selectedRows().size() == 1) &&
                       lvPhotos->selectionModel()->selectedRows().at(0).row());
  tbMoveDown->setEnabled((lvPhotos->selectionModel()->selectedRows().size() == 1) &&
                         (lvPhotos->selectionModel()->selectedRows().at(0).row() <
                          (core()->rowCount() - 1)));
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

  while (url.hasNext())
  {
    if (QFileInfo(url.peekNext().toLocalFile()).isDir())
      qobject_cast<Core *> (core())->addFiles(QDir(url.peekNext().toLocalFile()).entryInfoList(QDir::Files | QDir::Readable));
    else
      qobject_cast<Core *> (core())->addFile(url.peekNext().toLocalFile());

    url.next();
  }

  if (urls.size())
    emit settingsModified();
}
