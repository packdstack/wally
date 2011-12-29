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

#include "gui.h"
#include "defs.h"
#include "engine.h"
#include "settings.h"
#include "wally.h"

static QString positionToString(Wally::Application::Position position)
{
  switch (position)
  {
    case Wally::Application::Centered:
      return QObject::tr("Centered");

    case Wally::Application::Tiled:
      return QObject::tr("Tiled");

    case Wally::Application::CenterTiled:
      return QObject::tr("Center Tiled");

    case Wally::Application::CenteredMaxpect:
      return QObject::tr("Centered Maxpect");

    case Wally::Application::TiledMaxpect:
      return QObject::tr("Tiled Maxpect");

    case Wally::Application::Scaled:
      return QObject::tr("Scaled");

    case Wally::Application::CenteredAutoFit:
      return QObject::tr("Centered Auto Fit");

    case Wally::Application::ScaleAndCrop:
      return QObject::tr("Scale & Crop");

    case Wally::Application::SymmetricalTiled:
      return QObject::tr("Symmetrical Tiled");

    case Wally::Application::MirroredTiled:
      return QObject::tr("Mirrored Tiled");

    case Wally::Application::SymmetricalMirroredTiled:
      return QObject::tr("Symmetrical Mirrored Tiled");

    default:
      return QString();
  }
}

int PositionModel::rowCount(const QModelIndex & /* parent */) const
{
  return static_cast< int > (Wally::Application::SymmetricalMirroredTiled) + 1;
}

QVariant PositionModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || (role != Qt::DisplayRole))
    return QVariant();

  switch (index.column())
  {
    case 0:
      return positionToString(static_cast<Wally::Application::Position> (index.row()));
      break;

    case 1:
      switch (static_cast<Wally::Application::Position> (index.row()))
      {
        case Wally::Application::Centered:
          return ":/images/centered";

        case Wally::Application::Tiled:
          return ":/images/tiled";

        case Wally::Application::CenterTiled:
          return ":/images/center_tiled";

        case Wally::Application::CenteredMaxpect:
          return ":/images/centered_maxpect";

        case Wally::Application::TiledMaxpect:
          return ":/images/tiled_maxpect";

        case Wally::Application::Scaled:
          return ":/images/scaled";

        case Wally::Application::CenteredAutoFit:
          return ":/images/centered_autofit";

        case Wally::Application::ScaleAndCrop:
          return ":/images/scale_crop";

        case Wally::Application::SymmetricalTiled:
          return ":/images/symmetrical_tiled";

        case Wally::Application::MirroredTiled:
          return ":/images/mirrored_tiled";

        case Wally::Application::SymmetricalMirroredTiled:
          return ":/images/symmetrical_mirrored_tiled";
      }
  }

  return QVariant();
}

QVariant PositionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if ((orientation == Qt::Vertical) || (role != Qt::DisplayRole))
    return QVariant();

  switch (section)
  {
    case 0:
      return tr("Position");

    case 1:
      return tr("Picture smaller than screen on the left, greater than screen on the right");
  }

  return QVariant();
}

void PositionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QPixmap pixmap(index.data().toString());

  painter->setRenderHint(QPainter::Antialiasing,true);
  painter->fillRect(option.rect,(option.state & QStyle::State_Selected)? _palette.brush(QPalette::Highlight) :
                                                                         _palette.brush(QPalette::Base));
  painter->drawPixmap(option.rect.topLeft() + QPoint(2,2),pixmap);
}

QSize PositionDelegate::sizeHint(const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */) const
{
  return QImageReader(":/images/centered_maxpect").size() + QSize(5,5);
}

void PositionLabelDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QStyleOptionViewItem itemOption(option);
  itemOption.state = itemOption.state & ~QStyle::State_HasFocus;
  QItemDelegate::paint(painter,itemOption,index);
}

SettingsDialog::SettingsDialog(QWidget *parent) : Gui::Dialog(Gui::Dialog::CenterOfScreen,parent)
{
  setupUi(this);
  setWindowTitle(qApp->applicationName() + " " + qApp->applicationVersion() +
                 " - " + tr("Settings") + "[*]");

  firstShowEvent = true;
  loadingSettings = false;

  settingsButton = new Gui::EngineButton(tr("Settings"),sbToolbar);
  settingsButton->setChecked(true);

  QCoreApplication::translate("QDialogButtonBox","OK");
  QCoreApplication::translate("QDialogButtonBox","Cancel");
  QCoreApplication::translate("QDialogButtonBox","Reset");
  QCoreApplication::translate("QDialogButtonBox","&Yes");
  QCoreApplication::translate("QDialogButtonBox","&No");
  QCoreApplication::translate("QDialogButtonBox","Close");
  QCoreApplication::translate("QDialogButtonBox","&Close");

  QCoreApplication::translate("QColorDialog","Hu&e:");
  QCoreApplication::translate("QColorDialog","&Sat:");
  QCoreApplication::translate("QColorDialog","&Val:");
  QCoreApplication::translate("QColorDialog","&Red:");
  QCoreApplication::translate("QColorDialog","&Green:");
  QCoreApplication::translate("QColorDialog","Bl&ue:");
  QCoreApplication::translate("QColorDialog","Select Color");
  QCoreApplication::translate("QColorDialog","&Basic colors");
  QCoreApplication::translate("QColorDialog","&Custom colors");
  QCoreApplication::translate("QColorDialog","&Add to Custom Colors");

  QCoreApplication::translate("QFileDialog","Directories");
  QCoreApplication::translate("QFileDialog","&Open");
  QCoreApplication::translate("QFileDialog","&Save");
  QCoreApplication::translate("QFileDialog","Open");
  QCoreApplication::translate("QFileDialog","%1 already exists.\nDo you want to replace it?");
  QCoreApplication::translate("QFileDialog","%1\nFile not found.\nPlease verify the correct file name was given.");
  QCoreApplication::translate("QFileDialog","My Computer");
  QCoreApplication::translate("QFileDialog","&Rename");
  QCoreApplication::translate("QFileDialog","&Delete");
  QCoreApplication::translate("QFileDialog","Show &hidden files");
  QCoreApplication::translate("QFileDialog","Back");
  QCoreApplication::translate("QFileDialog","Parent Directory");
  QCoreApplication::translate("QFileDialog","List View");
  QCoreApplication::translate("QFileDialog","Detail View");
  QCoreApplication::translate("QFileDialog","Files of type:");
  QCoreApplication::translate("QFileDialog","Directory:");
  QCoreApplication::translate("QFileDialog","%1\nDirectory not found.\nPlease verify the correct directory name was given.");
  QCoreApplication::translate("QFileDialog","'%1' is write protected.\nDo you want to delete it anyway?");
  QCoreApplication::translate("QFileDialog","Are sure you want to delete '%1'?");
  QCoreApplication::translate("QFileDialog","Could not delete directory.");
  QCoreApplication::translate("QFileDialog","Recent Places");
  QCoreApplication::translate("QFileDialog","Save As");
  QCoreApplication::translate("QFileDialog","Drive");
  QCoreApplication::translate("QFileDialog","File");
  QCoreApplication::translate("QFileDialog","Unknown");
  QCoreApplication::translate("QFileDialog","Find Directory");
  QCoreApplication::translate("QFileDialog","Show");
  QCoreApplication::translate("QFileDialog","Forward");
  QCoreApplication::translate("QFileDialog","New Folder");
  QCoreApplication::translate("QFileDialog","&New Folder");
  QCoreApplication::translate("QFileDialog","&Choose");
  QCoreApplication::translate("QFileDialog","Remove");
  QCoreApplication::translate("QFileDialog","File &name:");
  QCoreApplication::translate("QFileDialog","Look in:");
  QCoreApplication::translate("QFileDialog","Create New Folder");

  sbToolbar->setExclusive(true);
  sbToolbar->insertButton(0,settingsButton,0);
  connect(sbToolbar,SIGNAL(buttonClicked(int)),swMain,SLOT(setCurrentIndex(int)));

  cbProxyType->addItem("HTTP",QNetworkProxy::HttpProxy);
  cbProxyType->addItem("SOCKS5",QNetworkProxy::Socks5Proxy);
  cbProxyType->setCurrentIndex(0);

  cbInfoPositionOnPhoto->addItem(tr("Left"),Qt::AlignLeft);
  cbInfoPositionOnPhoto->addItem(tr("Right"),Qt::AlignRight);
  cbInfoPositionOnPhoto->setCurrentIndex(1);

  cbFreeDiskSpaceFactor->addItem(tr("kBytes"),1);
  cbFreeDiskSpaceFactor->addItem(tr("MBytes"),1024);
  cbFreeDiskSpaceFactor->addItem(tr("GBytes"),1048576);
  cbFreeDiskSpaceFactor->setCurrentIndex(1);

  cbHistoryTimeLimitFactor->addItem(tr("day(s)",0,2));
  cbHistoryTimeLimitFactor->addItem(tr("month(s)",0,2));
  cbHistoryTimeLimitFactor->setCurrentIndex(0);

#ifdef Q_WS_WIN
  cbAutoLaunchOnSysStart->setEnabled(true);
  cbViewInfoInTooltip->setEnabled(false);
  cbViewInfoInTooltip->setChecked(false);
#endif

#ifdef Q_WS_MAC
  cbAutoLaunchOnSysStart->setEnabled(false);
  cbAutoLaunchOnSysStart->setChecked(false);
  cbViewInfoInTooltip->setEnabled(false);
  cbViewInfoInTooltip->setChecked(false);
#endif

#ifdef Q_WS_X11
  cbAutoLaunchOnSysStart->setEnabled(false);
  cbAutoLaunchOnSysStart->setChecked(false);
#endif

  connect(cbIntervalUnit,SIGNAL(currentIndexChanged(int)),this,SLOT(settingsModified()));
  connect(cbIntervalUnit,SIGNAL(currentIndexChanged(int)),this,SLOT(changeInterval()));
  connect(sbInterval,SIGNAL(valueChanged(int)),this,SLOT(settingsModified()));
  connect(rbDirectConnection,SIGNAL(toggled(bool)),this,SLOT(settingsModified()));
  connect(rbProxyConnection,SIGNAL(toggled(bool)),this,SLOT(settingsModified()));
  connect(leProxyPassword,SIGNAL(textChanged(const QString &)),this,SLOT(settingsModified()));
  connect(leProxyUsername,SIGNAL(textChanged(const QString &)),this,SLOT(settingsModified()));
  connect(cbProxyType,SIGNAL(currentIndexChanged(int)),this,SLOT(settingsModified()));
  connect(leProxyServer,SIGNAL(textChanged(const QString &)),this,SLOT(settingsModified()));
  connect(sbProxyPort,SIGNAL(valueChanged(int)),this,SLOT(settingsModified()));
  connect(cbAutoLaunchOnSysStart,SIGNAL(toggled(bool)),this,SLOT(settingsModified()));
  connect(cbAutoPlay,SIGNAL(toggled(bool)),this,SLOT(settingsModified()));
  connect(cbOnlyLandscapes,SIGNAL(toggled(bool)),this,SLOT(settingsModified()));
  connect(cbRandomOrder,SIGNAL(toggled(bool)),this,SLOT(settingsModified()));
  connect(cbSwitchOnPlay,SIGNAL(toggled(bool)),this,SLOT(settingsModified()));
  connect(cbDisableSplashScreen,SIGNAL(toggled(bool)),this,SLOT(settingsModified()));
  connect(cbQuitAfterBackgroundChange,SIGNAL(toggled(bool)),this,SLOT(settingsModified()));
  connect(cbUseFullDesktop,SIGNAL(toggled(bool)),this,SLOT(settingsModified()));
  connect(cbViewInfoInTooltip,SIGNAL(toggled(bool)),this,SLOT(settingsModified()));
  connect(cbViewInfoOnPhoto,SIGNAL(toggled(bool)),this,SLOT(settingsModified()));
  connect(cbInfoPositionOnPhoto,SIGNAL(currentIndexChanged(int)),this,SLOT(settingsModified()));
  connect(cbSizeConstraint,SIGNAL(currentIndexChanged(int)),this,SLOT(settingsModified()));
  connect(pbBorderColor,SIGNAL(colorChanged(const QColor &)),this,SLOT(settingsModified()));
  connect(sbFreeDiskSpace,SIGNAL(valueChanged(int)),this,SLOT(settingsModified()));
  connect(cbFreeDiskSpaceFactor,SIGNAL(currentIndexChanged(int)),this,SLOT(settingsModified()));
  connect(sbHistoryTimeLimit,SIGNAL(valueChanged(int)),this,SLOT(settingsModified()));
  connect(cbHistoryTimeLimitFactor,SIGNAL(currentIndexChanged(int)),this,SLOT(settingsModified()));
  connect(cbRotateImages,SIGNAL(toggled(bool)),this,SLOT(settingsModified()));

  cbIntervalUnit->addItem(tr("second(s)",0,2),SECONDS_UNIT);
  cbIntervalUnit->addItem(tr("minute(s)",0,2),MINUTES_UNIT);
  cbIntervalUnit->addItem(tr("hour(s)",0,2),HOURS_UNIT);
  cbIntervalUnit->setCurrentIndex(1);
  sbInterval->setRange(1,300);

  connect(lwAvailableModules,SIGNAL(itemSelectionChanged()),this,SLOT(listWidgetsSelectionChanged()));
  connect(lwActiveModules,SIGNAL(itemSelectionChanged()),this,SLOT(listWidgetsSelectionChanged()));

  swMain->setCurrentIndex(0);
}

void SettingsDialog::settingsModified()
{
  setWindowModified(!loadingSettings);
}

void SettingsDialog::addSettingsWidget(Engine::SettingsWidget *settingsWidget)
{
  QVariant value;
  QListWidgetItem *item;
  QListWidget *lwPtr;
  Gui::EngineButton *button = new Gui::EngineButton(settingsWidget->buttonIcon(),
                                                    settingsWidget->buttonText(),
                                                    sbToolbar);

  connect(settingsWidget,SIGNAL(settingsModified()),
          this,SLOT(settingsModified()));
  button->setPixmap(settingsWidget->buttonPixmap());
  sbToolbar->insertButton(0,button,swMain->addWidget(settingsWidget));
  settingsWidgets.push_back(settingsWidget);
  lwPtr = (settingsWidget->core()->isActive())? lwActiveModules : lwAvailableModules;
  item = new QListWidgetItem(settingsWidget->core()->name(),lwPtr);
  value.setValue(settingsWidget->core());
  item->setData(Qt::UserRole,value);
}

void SettingsDialog::loadSettings(const WallySettings &mainSettings)
{
  _settings = mainSettings;
  QColor borderColor = _settings.value(MAIN_SECTION).value(BORDER_COLOR).toString();
  borderColor.setAlpha(_settings.value(MAIN_SECTION).value(AUTO_COLOR).toBool()? 0 : 255);

  loadingSettings = true;
  sbInterval->setValue(_settings.value(MAIN_SECTION).value(INTERVAL).toInt());
  cbIntervalUnit->setCurrentIndex(cbIntervalUnit->findData(_settings.value(MAIN_SECTION).value(INTERVAL_UNIT)));
  pbBorderColor->setColor(borderColor);
  pbPosition->setProperty("wallPosition",_settings.value(MAIN_SECTION).value(WALLPAPER_POSITION));
  pbPosition->setText(positionToString(static_cast<Wally::Application::Position> (_settings.value(MAIN_SECTION).value(WALLPAPER_POSITION).toInt())).replace("&","&&"));
  cbSwitchOnPlay->setChecked(_settings.value(MAIN_SECTION).value(SWITCH_ON_PLAY).toBool());
  cbAutoPlay->setChecked(_settings.value(MAIN_SECTION).value(PLAY_ON_START).toBool());
  cbAutoLaunchOnSysStart->setChecked(_settings.value(MAIN_SECTION).value(RUN_ON_SYS_START).toBool());
  cbOnlyLandscapes->setChecked(_settings.value(MAIN_SECTION).value(ONLY_LANDSCAPES).toBool());
  cbRotateImages->setChecked(_settings.value(MAIN_SECTION).value(EXIF_ROTATE_IMAGES).toBool());
  cbSizeConstraint->setCurrentIndex(_settings.value(MAIN_SECTION).value(SIZE_CONSTRAINT).toInt());
  cbViewInfoInTooltip->setChecked(_settings.value(MAIN_SECTION).value(VIEW_INFO_IN_TOOLTIP).toBool());
  cbDisableSplashScreen->setChecked(_settings.value(MAIN_SECTION).value(DISABLE_SPLASH_SCREEN).toBool());
  cbQuitAfterBackgroundChange->setChecked(_settings.value(MAIN_SECTION).value(QUIT_AFTER_BACKGROUND_CHANGE).toBool());
  cbRandomOrder->setChecked(_settings.value(MAIN_SECTION).value(RANDOM_SEARCH).toBool());
  cbUseFullDesktop->setChecked(_settings.value(MAIN_SECTION).value(USE_FULL_DESKTOP_AREA).toBool());
  cbViewInfoOnPhoto->setChecked(_settings.value(MAIN_SECTION).value(VIEW_INFO_ON_PHOTO).toBool());
  cbInfoPositionOnPhoto->setCurrentIndex(
    cbInfoPositionOnPhoto->findData(_settings.value(MAIN_SECTION).value(INFO_POSITION_ON_PHOTO).toInt()));
  sbFreeDiskSpace->setValue(_settings.value(MAIN_SECTION).value(MIN_FREE_DISK_SPACE).toInt());
  cbFreeDiskSpaceFactor->setCurrentIndex(
    cbFreeDiskSpaceFactor->findData(_settings.value(MAIN_SECTION).value(MIN_FREE_DISK_SPACE_FACTOR).toInt()));
  sbHistoryTimeLimit->setValue(_settings.value(MAIN_SECTION).value(HISTORY_TIME_LIMIT).toInt());
  cbHistoryTimeLimitFactor->setCurrentIndex(_settings.value(MAIN_SECTION).value(HISTORY_TIME_LIMIT_FACTOR).toInt());
  rbDirectConnection->setChecked(!_settings.value(NETWORK_SECTION).value(USE_PROXY).toBool());
  rbProxyConnection->setChecked(_settings.value(NETWORK_SECTION).value(USE_PROXY).toBool());
  rbSystemProxy->setChecked(_settings.value(NETWORK_SECTION).value(USE_SYSTEM_PROXY).toBool());
  rbCustomProxy->setChecked(!_settings.value(NETWORK_SECTION).value(USE_SYSTEM_PROXY).toBool());
  cbProxyType->setCurrentIndex(cbProxyType->findData(_settings.value(NETWORK_SECTION).value(PROXY_TYPE).toInt()));
  leProxyServer->setText(_settings.value(NETWORK_SECTION).value(PROXY_SERVER).toString());
  sbProxyPort->setValue(_settings.value(NETWORK_SECTION).value(PROXY_PORT).toInt());
  cbProxyAuthentication->setChecked(_settings.value(NETWORK_SECTION).value(PROXY_AUTHENTICATION).toBool());
  leProxyUsername->setText(_settings.value(NETWORK_SECTION).value(PROXY_USERNAME).toString());
  leProxyPassword->setText(_settings.value(NETWORK_SECTION).value(PROXY_PASSWORD).toString());
  loadingSettings = false;
}

WallySettings SettingsDialog::settings()
{
  WallySettings newSettings;

  newSettings[MAIN_SECTION][DISABLE_SPLASH_SCREEN] = cbDisableSplashScreen->isChecked();
  newSettings[MAIN_SECTION][QUIT_AFTER_BACKGROUND_CHANGE] = cbQuitAfterBackgroundChange->isChecked();
  newSettings[MAIN_SECTION][INTERVAL] = sbInterval->value();
  newSettings[MAIN_SECTION][INTERVAL] = sbInterval->value();
  newSettings[MAIN_SECTION][INTERVAL_UNIT] = cbIntervalUnit->itemData(cbIntervalUnit->currentIndex()).toInt();
  newSettings[MAIN_SECTION][BORDER_COLOR] = pbBorderColor->color().name();
  newSettings[MAIN_SECTION][AUTO_COLOR] = !pbBorderColor->color().alpha();
  newSettings[MAIN_SECTION][WALLPAPER_POSITION] = static_cast<Wally::Application::Position> (pbPosition->property("wallPosition").toInt());
  newSettings[MAIN_SECTION][SWITCH_ON_PLAY] = cbSwitchOnPlay->isChecked();
  newSettings[MAIN_SECTION][PLAY_ON_START] = cbAutoPlay->isChecked();
  newSettings[MAIN_SECTION][RUN_ON_SYS_START] = cbAutoLaunchOnSysStart->isChecked();
  newSettings[MAIN_SECTION][EXIF_ROTATE_IMAGES] = cbRotateImages->isChecked();
  newSettings[MAIN_SECTION][ONLY_LANDSCAPES] = cbOnlyLandscapes->isChecked();
  newSettings[MAIN_SECTION][VIEW_INFO_IN_TOOLTIP] = cbViewInfoInTooltip->isChecked();
  newSettings[MAIN_SECTION][SIZE_CONSTRAINT] = cbSizeConstraint->currentIndex();
  newSettings[MAIN_SECTION][RANDOM_SEARCH] = cbRandomOrder->isChecked();
  newSettings[MAIN_SECTION][USE_FULL_DESKTOP_AREA] = cbUseFullDesktop->isChecked();
  newSettings[MAIN_SECTION][VIEW_INFO_ON_PHOTO] = cbViewInfoOnPhoto->isChecked();
  newSettings[MAIN_SECTION][INFO_POSITION_ON_PHOTO] = static_cast<Qt::AlignmentFlag>
    (cbInfoPositionOnPhoto->itemData(cbInfoPositionOnPhoto->currentIndex()).toInt());
  newSettings[MAIN_SECTION][MIN_FREE_DISK_SPACE] = sbFreeDiskSpace->value();
  newSettings[MAIN_SECTION][MIN_FREE_DISK_SPACE_FACTOR] =
    cbFreeDiskSpaceFactor->itemData(cbFreeDiskSpaceFactor->currentIndex()).toInt();
  newSettings[MAIN_SECTION][HISTORY_TIME_LIMIT] = sbHistoryTimeLimit->value();
  newSettings[MAIN_SECTION][HISTORY_TIME_LIMIT_FACTOR] = cbHistoryTimeLimitFactor->currentIndex();
  newSettings[NETWORK_SECTION][USE_PROXY] = rbProxyConnection->isChecked();
  newSettings[NETWORK_SECTION][USE_SYSTEM_PROXY] = rbSystemProxy->isChecked();
  newSettings[NETWORK_SECTION][PROXY_TYPE] = static_cast<QNetworkProxy::ProxyType>
    (cbProxyType->itemData(cbProxyType->currentIndex()).toInt());
  newSettings[NETWORK_SECTION][PROXY_SERVER] = leProxyServer->text();
  newSettings[NETWORK_SECTION][PROXY_PORT] = sbProxyPort->value();
  newSettings[NETWORK_SECTION][PROXY_AUTHENTICATION] = cbProxyAuthentication->isChecked();
  newSettings[NETWORK_SECTION][PROXY_USERNAME] = leProxyUsername->text();
  newSettings[NETWORK_SECTION][PROXY_PASSWORD] = leProxyPassword->text();

  return newSettings;
}

void SettingsDialog::showEvent(QShowEvent *event)
{
  if (firstShowEvent)
  {
    int realHeight = 0;

    QListIterator<QAbstractButton *> button(sbToolbar->buttons());
    while (button.hasNext())
      realHeight = qMax(realHeight,button.next()->height());

    QList<QAbstractButton *> buttons = sbToolbar->buttons();
    QMutableListIterator<QAbstractButton *> mButton(buttons);

    while (mButton.hasNext())
      qobject_cast<Gui::EngineButton *> (mButton.next())->setHeight(realHeight + 10);

    listWidgetsSelectionChanged();

    firstShowEvent = false;
  }

  Gui::Dialog::showEvent(event);
}

void SettingsDialog::on_pbPosition_clicked()
{
  QScopedPointer< Gui::Dialog > dialog(new Gui::Dialog(Gui::Dialog::CenterOfScreen,this));
  PositionModel *model = new PositionModel(dialog.data());
  QTableView *view = new QTableView(dialog.data());
  QItemSelectionModel *selectionModel = new QItemSelectionModel(model,dialog.data());

  dialog->setWindowTitle(tr("Set position"));

  QVBoxLayout *layout = new QVBoxLayout;
  QDialogButtonBox *dialogButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                         Qt::Horizontal,dialog.data());
  connect(dialogButtons,SIGNAL(accepted()),dialog.data(),SLOT(accept()));
  connect(dialogButtons,SIGNAL(rejected()),dialog.data(),SLOT(reject()));

  view->setModel(model);
  view->setItemDelegateForColumn(0,new PositionLabelDelegate);
  view->setItemDelegateForColumn(1,new PositionDelegate(QApplication::palette()));
  view->setSelectionBehavior(QAbstractItemView::SelectRows);
  view->setSelectionMode(QAbstractItemView::SingleSelection);
  view->setSelectionModel(selectionModel);
  view->verticalHeader()->setVisible(false);
  view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  view->setEditTriggers(QAbstractItemView::NoEditTriggers);

  layout->addWidget(view);
  layout->addWidget(dialogButtons);
  dialog->setLayout(layout);
  dialog->resize(700,500);
  connect(dialog.data(),SIGNAL(executed()),view,SLOT(resizeColumnsToContents()));
  connect(dialog.data(),SIGNAL(executed()),view,SLOT(resizeRowsToContents()));

  selectionModel->select(model->index(pbPosition->property("wallPosition").toInt(),0),
                         QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
  view->scrollTo(model->index(pbPosition->property("wallPosition").toInt(),0));
  if (dialog->exec() == QDialog::Accepted)
  {
    Wally::Application::Position p = static_cast<Wally::Application::Position> (selectionModel->currentIndex().row());
    pbPosition->setProperty("wallPosition",static_cast<int> (p));
    pbPosition->setText(positionToString(p).replace("&","&&"));
    settingsModified();
  }
}

void SettingsDialog::on_rbProxyConnection_toggled(bool checked)
{
  wdgProxy->setEnabled(checked);
  wdgProxyServer->setEnabled(rbCustomProxy->isChecked());
  wdgProxyAuthentication->setEnabled(cbProxyAuthentication->isChecked());
}

void SettingsDialog::on_bbResult_clicked(QAbstractButton *button)
{
  QListIterator<Engine::SettingsWidget *> settingsWidget(settingsWidgets);

  if (bbResult->buttonRole(button) == QDialogButtonBox::ResetRole)
  {
    loadSettings(_settings);

    while (settingsWidget.hasNext())
      settingsWidget.next()->core()->revert();
  }
}

void SettingsDialog::done(int r)
{
  if ((r != QDialog::Rejected) || !isWindowModified() ||
      (QMessageBox::question(this,tr("Settings"),tr("Changes won't be applied. Are you sure?"),
                             QMessageBox::Yes | QMessageBox::No,QMessageBox::No) != QMessageBox::No))
  {
    QDialog::done(r);
    close();
  }
}

void SettingsDialog::on_sbInterval_valueChanged(int i)
{
  cbIntervalUnit->setItemText(cbIntervalUnit->findData(SECONDS_UNIT),tr("second(s)",0,i));
  cbIntervalUnit->setItemText(cbIntervalUnit->findData(MINUTES_UNIT),tr("minute(s)",0,i));
  cbIntervalUnit->setItemText(cbIntervalUnit->findData(HOURS_UNIT),tr("hour(s)",0,i));
}

void SettingsDialog::listWidgetsSelectionChanged()
{
  tbRemoveAll->setEnabled(lwActiveModules->count());
  tbAddAll->setEnabled(lwAvailableModules->count());
  tbRemove->setEnabled(lwActiveModules->selectedItems().size());
  tbAdd->setEnabled(lwAvailableModules->selectedItems().size());
}

QMap<Engine::Core *, bool> SettingsDialog::activations() const
{
  int r;
  QMap<Engine::Core *, bool> result;

  for (r = 0; r < lwAvailableModules->count(); ++r)
    result[lwAvailableModules->item(r)->data(Qt::UserRole).value<Engine::Core *>()] = false;

  for (r = 0; r < lwActiveModules->count(); ++r)
    result[lwActiveModules->item(r)->data(Qt::UserRole).value<Engine::Core *>()] = true;

  return result;
}

void SettingsDialog::on_tbRemoveAll_clicked()
{
  int r;

  for (r = lwActiveModules->count() - 1; r >= 0; --r)
    lwAvailableModules->addItem(lwActiveModules->takeItem(r));

  listWidgetsSelectionChanged();
  settingsModified();
}

void SettingsDialog::on_tbRemove_clicked()
{
  int i;
  QList<QListWidgetItem *> items = lwActiveModules->selectedItems();

  for (i = 0; i < items.size(); ++i)
    lwAvailableModules->addItem(lwActiveModules->takeItem(lwActiveModules->row(items.at(i))));

  listWidgetsSelectionChanged();
  settingsModified();
}

void SettingsDialog::on_tbAddAll_clicked()
{
  int r;

  for (r = lwAvailableModules->count() - 1; r >= 0; --r)
    lwActiveModules->addItem(lwAvailableModules->takeItem(r));

  listWidgetsSelectionChanged();
  settingsModified();
}

void SettingsDialog::on_tbAdd_clicked()
{
  int i;
  QList<QListWidgetItem *> items = lwAvailableModules->selectedItems();

  for (i = 0; i < items.size(); ++i)
    lwActiveModules->addItem(lwAvailableModules->takeItem(lwAvailableModules->row(items.at(i))));

  listWidgetsSelectionChanged();
  settingsModified();
}

void SettingsDialog::on_pbClearHistory_clicked()
{
  if (QMessageBox::question(this,tr("Clear history"),tr("Are you sure?"),QMessageBox::Yes | QMessageBox::No,QMessageBox::No) == QMessageBox::Yes)
    emit clearHistory();
}

void SettingsDialog::changeInterval()
{
  switch (cbIntervalUnit->itemData(cbIntervalUnit->currentIndex()).toInt())
  {
    case SECONDS_UNIT:
      sbInterval->setRange(10,300);
      break;

    case MINUTES_UNIT:
    case HOURS_UNIT:
      sbInterval->setRange(1,300);
      break;
  }
}
