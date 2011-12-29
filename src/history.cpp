/*
TRANSLATOR History::EngineQueryModel
*/
/*
TRANSLATOR History::TagsQueryModel
*/
/*
TRANSLATOR History::PhotosQueryModel
*/
/*
TRANSLATOR History::Dialog
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

#include "networkmanager.h"
#include "defs.h"
#include "exif.h"
#include "engine.h"
#include "viewer.h"
#include "history.h"

using namespace History;

bool SortModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
  if (!sourceModel()->data(left,Qt::UserRole).toString().compare("All",Qt::CaseInsensitive))
    return true;
  else if (!sourceModel()->data(right,Qt::UserRole).toString().compare("All",Qt::CaseInsensitive))
    return false;
  else
    return (sourceModel()->data(left,Qt::UserRole).toString() <
            sourceModel()->data(right,Qt::UserRole).toString());
}

EngineQueryModel::EngineQueryModel(const QSqlDatabase &db, QObject *parent) : QSqlQueryModel(parent)
{
  setQuery(QString("select 'All' as engine, count(*) as photos from photoHistory") +
                   " union select engine, count(*) as photos from photoHistory group by engine",db);
}

QVariant EngineQueryModel::data(const QModelIndex &item, int role) const
{
  QString value;

  if (!item.column())
    switch (role)
    {
      case Qt::DisplayRole:
        if (!record(item.row()).value("engine").toString().compare("All",Qt::CaseInsensitive))
          value = tr("All");
        else
          value = record(item.row()).value("engine").toString();

        value += QString(" (") + record(item.row()).value("photos").toString() + ")";
        return value;

      case Qt::UserRole:
        return record(item.row()).value("engine").toString();
    }

  return QSqlQueryModel::data(item,role);
}

TagsQueryModel::TagsQueryModel(const QSqlDatabase &db, QObject *parent) :
  QSqlQueryModel(parent), _db(db)
{
  selectEngine("All");
}

QVariant TagsQueryModel::data(const QModelIndex &item, int role) const
{
  QString value;

  if (!item.column())
    switch (role)
    {
      case Qt::DisplayRole:
        if (!record(item.row()).value("tags").toString().compare("All",Qt::CaseInsensitive))
          value = tr("All");
        else
          value = record(item.row()).value("tags").toString();

        value += QString(" (") + record(item.row()).value("tagsCount").toString() + ")";
        return value;

      case Qt::UserRole:
        return record(item.row()).value("tags").toString();
    }

  return QSqlQueryModel::data(item,role);
}

void TagsQueryModel::selectEngine(const QString &engineName)
{
  if (!engineName.compare("All",Qt::CaseInsensitive))
    setQuery(QString("select 'All' as tags, count(*) as tagsCount from photoHistory") +
                     " union select tags, count(*) as tagsCount from photoHistory group by tags",_db);
  else
    setQuery(QString("select 'All' as tags, count(*) as tagsCount from photoHistory where engine = '") + engineName + "' " +
                     " union select tags, count(*) as tagsCount from photoHistory where engine = '" + engineName + "' group by tags",_db);
}

PhotosQueryModel::PhotosQueryModel(const QSqlDatabase &db, QObject *parent) :
  QSqlQueryModel(parent), _db(db)
{
  connect(&sizeHintTimer,SIGNAL(timeout()),this,SLOT(calculateSizeHints()));
}

QVariant PhotosQueryModel::data(const QModelIndex &item, int role) const
{
  QVariant var;
  QWidget *widget;
  QSize sizeHint;

  if (item.isValid())
    switch (role)
    {
      case Qt::DisplayRole:
        return formatCaption(record(item.row()));

      case Qt::ToolTipRole:
        if (!record(item.row()).value("description").toString().isEmpty())
          return QString("<i>") + record(item.row()).value("description").toString() + "</i>";
        else
          return QVariant();

      case Qt::SizeHintRole:
        if (sizeHints.contains(record(item.row()).value("timestamp").toInt()))
          return sizeHints.value(record(item.row()).value("timestamp").toInt());
        else
          return QSize(100,100);

      case Qt::UserRole:
        return record(item.row()).value("timestamp").toInt();

      case Qt::UserRole+1:
        if (sizeHints.contains(record(item.row()).value("timestamp").toInt()))
          var.setValue(createWidget(record(item.row())));
        else
        {
          widget = createWidget(record(item.row()));
          sizeHints[record(item.row()).value("timestamp").toInt()] = widget->sizeHint();
          delete widget;

          var.setValue(0);
        }

        return var;
    }

  return QSqlQueryModel::data(item,role);
}

void PhotosQueryModel::selectData(const QString &engineName, const QString &tags)
{
  QString dataQuery = "select * from photoHistory ";

  if (engineName.compare("All",Qt::CaseInsensitive) ||
      tags.compare("All",Qt::CaseInsensitive))
    dataQuery += "where ";

  if (engineName.compare("All",Qt::CaseInsensitive))
    dataQuery += "engine = '" + engineName + "' ";

  if (tags.compare("All",Qt::CaseInsensitive))
  {
    if (engineName.compare("All",Qt::CaseInsensitive))
      dataQuery += "and ";

    dataQuery += "tags = '" + tags + "' ";
  }

  dataQuery += "order by timestamp desc";

  setQuery(dataQuery,_db);
}

void PhotosQueryModel::queryChange()
{
  sizeHints.clear();

  QSqlQueryModel::queryChange();
}

void PhotosQueryModel::calculateSizeHints()
{
  if (currentQuery.next())
  {
    QWidget *widget = createWidget(currentQuery.record());
    sizeHints[currentQuery.record().value("timestamp").toInt()] = widget->sizeHint();
    delete widget;
  }
  else
  {
    sizeHintTimer.stop();
    reset();
  }
}

QString PhotosQueryModel::formatCaption(const QSqlRecord &itemRecord) const
{
  QString text;
  int timestampNo, titleNo, ownerNo, sizeNo, engineNo, locationNo, exifNo,
      widthNo, heightNo, timestamp;

  timestampNo = itemRecord.indexOf("timestamp");
  titleNo = itemRecord.indexOf("title");
  ownerNo = itemRecord.indexOf("owner");
  sizeNo = itemRecord.indexOf("size");
  engineNo = itemRecord.indexOf("engine");
  locationNo = itemRecord.indexOf("location");
  exifNo = itemRecord.indexOf("exif");
  widthNo = itemRecord.indexOf("width");
  heightNo = itemRecord.indexOf("height");

  timestamp = itemRecord.value(timestampNo).toInt();
  text = QString("<html><head></head><body><b>");
  text += ((itemRecord.value(titleNo).toString().isEmpty())? tr("(no title)") :
           itemRecord.value(titleNo).toString()) + "</b><br><table cellspacing=\"3\">";
  text += QString("<tr><td align=\"right\"><i>") + tr("by:") + "</i></td><td>" +
          ((itemRecord.value(ownerNo).toString().isEmpty())? tr("(no author)") :
           itemRecord.value(ownerNo).toString()) + "</td></tr>";
  text += QString("<tr><td align=\"right\"><i>") + tr("Date:") + "</i></td><td>" +
          QDateTime::fromTime_t(timestamp).toString(Qt::SystemLocaleLongDate) + "</td></tr>";
  text += QString("<tr><td align=\"right\"><i>") + tr("Location:") + "</i></td><td>" +
          ((itemRecord.value(locationNo).toString().isEmpty())? tr("unknown") :
           itemRecord.value(locationNo).toString()) + "</td></tr></table>";
  text += QString("<table cellspacing=\"10\"><tr><td><i>") + tr("Size:") + "</i>&nbsp;" +
          QString::number(itemRecord.value(sizeNo).toInt() / 1024) + "k</td><td><i>" +
          tr("Image size:") + "</i>&nbsp;" + QString::number(itemRecord.value(widthNo).toInt()) + "x" +
          QString::number(itemRecord.value(heightNo).toInt()) + "</td><td><i>" +
          tr("Engine:") + "</i>&nbsp;" + itemRecord.value(engineNo).toString() + "</td><td><i>" +
          tr("EXIF:") + "</i>&nbsp;" + ((itemRecord.isNull(exifNo))? tr("No") : tr("Yes")) +
          "</td></tr></table></body></html>";

  return text;
}

QWidget *PhotosQueryModel::createWidget(const QSqlRecord &itemRecord) const
{
  QWidget *widget;
  QHBoxLayout *layout;
  QLabel *label;
  QLabel *thumbnail;

  widget = new QWidget;
  widget->setAutoFillBackground(true);
  layout = new QHBoxLayout;

  thumbnail = new QLabel(widget);
  thumbnail->setFixedSize(100,100);
  thumbnail->setAutoFillBackground(true);
  thumbnail->setAlignment(Qt::AlignCenter);
  label = new QLabel(widget);
  label->setTextFormat(Qt::RichText);
  label->setAutoFillBackground(true);
  label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  layout->addWidget(thumbnail);
  layout->addWidget(label);
  layout->setContentsMargins(3,3,3,3);
  layout->setSpacing(10);

  widget->setLayout(layout);

  label->setText(formatCaption(itemRecord));
  thumbnail->setPixmap(QPixmap(QFileInfo(_db.databaseName()).absolutePath() + "/" +
                               QString::number(itemRecord.value("timestamp").toInt()) + ".png"));

  return widget;
}

void PhotoThumbnail::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
  if (index.isValid())
  {
    QWidget *widget = index.data(Qt::UserRole+1).value<QWidget *>();

    if (widget)
    {
      widget->setPalette(_palette);
      widget->setBackgroundRole((option.state & QStyle::State_Selected)? QPalette::Highlight :
                                ((!(index.row() % 2))? QPalette::Base : QPalette::AlternateBase));
      widget->setForegroundRole((option.state & QStyle::State_Selected)? QPalette::HighlightedText :
                                                                         QPalette::Text);

      painter->save();
      painter->setRenderHint(QPainter::Antialiasing,true);
      painter->drawPixmap(option.rect.left(),option.rect.top(),
                          QPixmap::grabWidget(widget,
                                              QRect(0,0,option.rect.width(),
                                                    option.rect.height())));
      painter->restore();

      delete widget;
    }
    else
      emit const_cast<PhotoThumbnail *> (this)->sizeHintChanged(index);
  }
  else
    return QItemDelegate::paint(painter,option,index);
}

Dialog::Dialog(const QSqlDatabase &db, const QString &tempStorageDir, QWidget *parent) :
  Gui::Dialog(Gui::Dialog::CenterOfScreen,parent), request(reqFile), _db(db),
  httpProxy(QNetworkProxy::NoProxy)
{
  QList<int> sizes;
  QPalette palette(QApplication::palette());
  QSplitter *splitter = new QSplitter(this);
  QVBoxLayout *vLayout = new QVBoxLayout;
  QSplitter *leftSplitter = new QSplitter(Qt::Vertical,this);
  QWidget *rightWidget = new QWidget(this);
  QVBoxLayout *vRightLayout = new QVBoxLayout;
  QLabel *hintLabel = new QLabel(this);

  _tempStorageDir = tempStorageDir;

  QCoreApplication::translate("QProgressDialog","Cancel");

  vLayout->setMargin(0);
  splitter->setOrientation(Qt::Horizontal);

  enginesView = new QListView(this);
  enginesView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  enginesView->setDragDropMode(QAbstractItemView::NoDragDrop);
  enginesView->setAlternatingRowColors(true);

  tagsView = new QListView(this);
  tagsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  tagsView->setDragDropMode(QAbstractItemView::NoDragDrop);
  tagsView->setAlternatingRowColors(true);

  photosView = new QListView(this);
  photosView->setContextMenuPolicy(Qt::ActionsContextMenu);
  photosView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  photosView->setDragDropMode(QAbstractItemView::NoDragDrop);
  photosView->setAlternatingRowColors(true);
  photosView->setResizeMode(QListView::Adjust);
  photosView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  photosView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

  hintLabel->setText(tr("Right-click on items to show options"));
  hintLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

  vRightLayout->setContentsMargins(0,0,0,0);
  vRightLayout->addWidget(photosView);
  vRightLayout->addWidget(hintLabel);
  rightWidget->setLayout(vRightLayout);

  leftSplitter->addWidget(enginesView);
  leftSplitter->addWidget(tagsView);

  splitter->addWidget(leftSplitter);
  splitter->addWidget(rightWidget);
  vLayout->addWidget(splitter);
#ifdef Q_WS_MAC
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok,Qt::Horizontal,this);

  connect(buttonBox,SIGNAL(accepted()),this,SLOT(close()));

  vLayout->addWidget(buttonBox);
#endif
  setLayout(vLayout);

  palette.setColor(QPalette::Highlight,Qt::darkCyan);
  setPalette(palette);

  splitter->setStretchFactor(1,4);
  sizes << (width() / 4);
  sizes << (3 * width() / 4);
  splitter->setSizes(sizes);

  setWindowTitle(tr("History"));
  setWindowIcon(QIcon(":/images/history"));
  resize(800,600);

  initializeModelsAndViews();

  initializeActions();

  initHttp();
}

void Dialog::changeTagsList(const QModelIndex &current, const QModelIndex & /* previous */)
{
  if (current.isValid())
  {
    tagsQueryModel->selectEngine(current.model()->data(current,Qt::UserRole).toString());
    sortedTagsSelectionModel->setCurrentIndex(sortedTagsSelectionModel->model()->index(0,0),QItemSelectionModel::Select);
    photosView->scrollToTop();
  }
}

void Dialog::changePhotosView(const QModelIndex &current, const QModelIndex & /* previous */)
{
  if (current.isValid())
  {
    photosQueryModel->selectData(sortedEnginesQueryModel->data(sortedEnginesSelectionModel->currentIndex(),Qt::UserRole).toString(),
                                 sortedTagsQueryModel->data(sortedTagsSelectionModel->currentIndex(),Qt::UserRole).toString());
    photosView->scrollToTop();
  }
}

void Dialog::updateContextMenuState(const QModelIndex &current, const QModelIndex & /* previous */)
{
  if (current.isValid())
  {
    QUrl url = photosQueryModel->record(current.row()).value("sourceUrl").toString();

    exploreSourceAction->setEnabled(!url.isEmpty() && url.isValid());
  }
}

QProgressDialog *Dialog::newHttpProgressDialog(QWidget *parent)
{
  QProgressDialog *result = new QProgressDialog(parent);

  result->setWindowTitle(tr("Please wait..."));
  result->setLabelText(tr("Downloading photo..."));
  result->setCancelButtonText(tr("Abort"));
  result->setAutoClose(true);

  return result;
}

void Dialog::execSavePhoto()
{
  int row = photosSelectionModel->currentIndex().row();
  QUrl url = photosQueryModel->record(row).value("url").toString();

#ifdef Q_WS_X11
  fileName = QFileDialog::getSaveFileName(0,tr("Save photo"),
                                          QDir::homePath() + "/" + QFileInfo(url.path()).fileName(),
                                          tr("Images (*.png *.xpm *.jpg)"),0,QFileDialog::DontUseNativeDialog);
#else
  fileName = QFileDialog::getSaveFileName(0,tr("Save photo"),
                                          QDesktopServices::storageLocation(QDesktopServices::PicturesLocation) +
                                          "/" + QFileInfo(url.path()).fileName(),
                                          tr("Images (*.png *.xpm *.jpg)"),0,QFileDialog::DontUseNativeDialog);
#endif

  if (!fileName.isEmpty())
  {
    httpProgressDialog = newHttpProgressDialog(this);

    request = reqFile;
    if (!httpProxy.hostName().compare("0.0.0.0"))
      http->setProxy(guessProxy(url));
    else
      http->setProxy(httpProxy);

    Network::Reply *reply = http->get(url,fileName);

    connect(httpProgressDialog,SIGNAL(canceled()),reply,SLOT(abort()));
    connect(reply,SIGNAL(finished(bool,const QByteArray &)),httpProgressDialog,SLOT(reset()));
    connect(reply,SIGNAL(finished(bool,const QByteArray &)),this,SLOT(httpDone(bool,const QByteArray &)));
    connect(reply,SIGNAL(progress(qint64,qint64)),this,SLOT(httpDownloading(qint64,qint64)));

    httpProgressDialog->show();
  }
}

void Dialog::execSetAsBackground()
{
  int row = photosSelectionModel->currentIndex().row();
  QUrl url = photosQueryModel->record(row).value("url").toString();

  httpProgressDialog = newHttpProgressDialog(this);

  fileName = _tempStorageDir + "/" + QFileInfo(url.path()).fileName();

  request = reqWallpaper;
  if (!httpProxy.hostName().compare("0.0.0.0"))
    http->setProxy(guessProxy(url));
  else
    http->setProxy(httpProxy);

  Network::Reply *reply = http->get(url,fileName);

  connect(httpProgressDialog,SIGNAL(canceled()),reply,SLOT(abort()));
  connect(reply,SIGNAL(finished(bool,const QByteArray &)),httpProgressDialog,SLOT(reset()));
  connect(reply,SIGNAL(finished(bool,const QByteArray &)),this,SLOT(httpDone(bool,const QByteArray &)));
  connect(reply,SIGNAL(progress(qint64,qint64)),this,SLOT(httpDownloading(qint64,qint64)));

  httpProgressDialog->show();
}

void Dialog::execGetExifInfo()
{
  QByteArray tagsData;
  int row = photosSelectionModel->currentIndex().row();

  if (!photosQueryModel->record(row).isNull("exif"))
  {
    tagsData = qUncompress(photosQueryModel->record(row).value("exif").toByteArray());
    if (tagsData.isEmpty())
      tagsData = photosQueryModel->record(row).value("exif").toByteArray();
  }

  Exif::InfoDialog(tagsData).exec();
}

void Dialog::execViewPhoto()
{
  int row = photosSelectionModel->currentIndex().row();
  QUrl url = photosQueryModel->record(row).value("url").toString();

  httpProgressDialog = newHttpProgressDialog(this);
  fileName = _tempStorageDir + "/" + QFileInfo(url.path()).fileName();

  request = reqView;
  if (!httpProxy.hostName().compare("0.0.0.0"))
    http->setProxy(guessProxy(url));
  else
    http->setProxy(httpProxy);

  Network::Reply *reply = http->get(url,fileName);

  connect(httpProgressDialog,SIGNAL(canceled()),reply,SLOT(abort()));
  connect(reply,SIGNAL(finished(bool,const QByteArray &)),httpProgressDialog,SLOT(reset()));
  connect(reply,SIGNAL(finished(bool,const QByteArray &)),this,SLOT(httpDone(bool,const QByteArray &)));
  connect(reply,SIGNAL(progress(qint64,qint64)),this,SLOT(httpDownloading(qint64,qint64)));

  httpProgressDialog->show();
}

void Dialog::execExploreImageSource()
{
  QModelIndex index = photosSelectionModel->currentIndex();

  if (index.isValid())
  {
    QUrl url = photosQueryModel->record(index.row()).value("sourceUrl").toString();
    QDesktopServices::openUrl(url);
  }
}

void Dialog::httpDownloading(qint64 done, qint64 total)
{
  if (!httpProgressDialog->wasCanceled())
  {
    httpProgressDialog->setRange(0,total);
    httpProgressDialog->setValue(done);
  }
}

void Dialog::httpDone(bool ok, const QByteArray & /* data */)
{
  Viewer *viewer;
  PhotoInfo info;
  int row;
  bool canceled = httpProgressDialog->wasCanceled();

  httpProgressDialog->reset();
  row = photosSelectionModel->currentIndex().row();

  if (ok)
  {
    switch (request)
    {
      case reqWallpaper:
        emit photoDownloaded(true,QFileInfo(fileName));

        info.owner = photosQueryModel->record(row).value("owner").toString();
        info.title = photosQueryModel->record(row).value("title").toString();
        info.description = photosQueryModel->record(row).value("description").toString();
        info.location = photosQueryModel->record(row).value("location").toString();

        emit infoCollected(info);
        emit changePhoto(photosQueryModel->record(row).value("engine").toString());
        break;

      case reqFile:
        QMessageBox::information(this,tr("History"),tr("Photo has been saved"));
        break;

      case reqView:
        viewer = new Viewer(this);

        viewer->setPixmap(QPixmap(fileName));
        viewer->setWindowTitle(photosQueryModel->record(row).value("title").toString());

        viewer->exec();

        delete viewer;
        break;
    }
  }
  else
    if (!canceled)
      QMessageBox::critical(this,tr("History"),tr("There was an error during download"));

  httpProgressDialog->deleteLater();
  httpProgressDialog = 0;

  sender()->deleteLater();
}

void Dialog::initializeModelsAndViews()
{
  PhotoThumbnail *thumbnail = new PhotoThumbnail(palette(),this);

  enginesQueryModel = new EngineQueryModel(_db,this);
  sortedEnginesQueryModel = new SortModel(this);
  sortedEnginesQueryModel->setDynamicSortFilter(true);
  sortedEnginesQueryModel->setSourceModel(enginesQueryModel);
  sortedEnginesQueryModel->sort(0);
  enginesView->setModel(sortedEnginesQueryModel);
  sortedEnginesSelectionModel = new QItemSelectionModel(sortedEnginesQueryModel,this);
  enginesView->setSelectionModel(sortedEnginesSelectionModel);
  connect(sortedEnginesSelectionModel,SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
          this,SLOT(changePhotosView(const QModelIndex &, const QModelIndex &)));
  connect(sortedEnginesSelectionModel,SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
          this,SLOT(changeTagsList(const QModelIndex &, const QModelIndex &)));

  tagsQueryModel = new TagsQueryModel(_db,this);
  sortedTagsQueryModel = new SortModel(this);
  sortedTagsQueryModel->setDynamicSortFilter(true);
  sortedTagsQueryModel->setSourceModel(tagsQueryModel);
  sortedTagsQueryModel->sort(0);
  tagsView->setModel(sortedTagsQueryModel);
  sortedTagsSelectionModel = new QItemSelectionModel(sortedTagsQueryModel,this);
  tagsView->setSelectionModel(sortedTagsSelectionModel);
  connect(sortedTagsSelectionModel,SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
          this,SLOT(changePhotosView(const QModelIndex &, const QModelIndex &)));

  photosQueryModel = new PhotosQueryModel(_db,this);
  photosView->setModel(photosQueryModel);
  photosView->setItemDelegate(thumbnail);
  photosSelectionModel = new QItemSelectionModel(photosQueryModel,this);
  photosView->setSelectionModel(photosSelectionModel);
  connect(photosSelectionModel,SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
          this,SLOT(updateContextMenuState(const QModelIndex &, const QModelIndex &)));

  sortedEnginesSelectionModel->setCurrentIndex(sortedEnginesSelectionModel->model()->index(0,0),QItemSelectionModel::Select);
  sortedTagsSelectionModel->setCurrentIndex(sortedTagsSelectionModel->model()->index(0,0),QItemSelectionModel::Select);
}

void Dialog::initializeActions()
{
  QAction *action = new QAction(QIcon(":/images/view_photo"),tr("View photo..."),this);
  connect(action,SIGNAL(triggered()),this,SLOT(execViewPhoto()));
  photosView->addAction(action);

  action = new QAction(QIcon(":images/exif_info"),tr("Get EXIF info..."),this);
  connect(action,SIGNAL(triggered()),this,SLOT(execGetExifInfo()));
  photosView->addAction(action);

  action = new QAction(QIcon(":/images/photo_save"),tr("Save photo..."),this);
  connect(action,SIGNAL(triggered()),this,SLOT(execSavePhoto()));
  photosView->addAction(action);

  action = new QAction(QIcon(":/images/photo_background"),tr("Set as background"),this);
  connect(action,SIGNAL(triggered()),this,SLOT(execSetAsBackground()));
  photosView->addAction(action);

  exploreSourceAction = new QAction(QIcon(":/images/source_url"),tr("Explore image source"),this);
  connect(exploreSourceAction,SIGNAL(triggered()),this,SLOT(execExploreImageSource()));
  photosView->addAction(exploreSourceAction);
}

void Dialog::setProxy(const QNetworkProxy &proxy)
{
  qDebug() << "Dialog::setProxy(...)";

  httpProxy = proxy;
}

QNetworkProxy Dialog::guessProxy(const QUrl &url)
{
  QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery(QNetworkProxyQuery(url));

  if (proxies.size())
  {
    QNetworkProxy localProxy = proxies.at(0);
    localProxy.setUser(httpProxy.user());
    localProxy.setPassword(httpProxy.password());

    return localProxy;
  }
  else
    return QNetworkProxy(QNetworkProxy::NoProxy);
}

void Dialog::initHttp()
{
  httpProgressDialog = 0;
  http = new Network::Manager(this);
}
