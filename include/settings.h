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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtCore>
#include <QtGui>
#include <QtNetwork>
#include "ui_settings.h"

#include "gui.h"
#include "defs.h"
#include "engine.h"

class PositionModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  PositionModel(QObject *parent = 0) : QAbstractTableModel(parent) { }

  int columnCount(const QModelIndex & /* parent */ = QModelIndex()) const { return 2; }
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  int rowCount(const QModelIndex & /* parent */ = QModelIndex()) const;
};

class PositionDelegate : public QItemDelegate
{
  QPalette _palette;

public:
  PositionDelegate(const QPalette &palette, QObject *parent = 0) : QItemDelegate(parent), _palette(palette) { }

  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class PositionLabelDelegate : public QItemDelegate
{
public:
  PositionLabelDelegate(QObject *parent = 0) : QItemDelegate(parent) { }

  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class SettingsDialog : public Gui::Dialog, private Ui::SettingsDialog
{
  Q_OBJECT

  bool firstShowEvent;
  bool loadingSettings;
  WallySettings _settings;
  Gui::EngineButton *settingsButton;
  QList<Engine::SettingsWidget *> settingsWidgets;

private slots:
  void on_bbResult_clicked(QAbstractButton *button);
  void on_sbInterval_valueChanged(int i);
  void on_rbProxyConnection_toggled(bool checked);
  void settingsModified();
  void listWidgetsSelectionChanged();
  void on_tbRemoveAll_clicked();
  void on_tbRemove_clicked();
  void on_tbAddAll_clicked();
  void on_tbAdd_clicked();
  void on_pbClearHistory_clicked();
  void changeInterval();
  void on_pbPosition_clicked();

protected:
  void showEvent(QShowEvent *event);

public:
  SettingsDialog(QWidget *parent = 0);
  virtual ~SettingsDialog() { }

  void addSettingsWidget(Engine::SettingsWidget *settingsWidget);

  void loadSettings(const WallySettings &mainSettings);

  WallySettings settings();
  QMap<Engine::Core *, bool> activations() const;

signals:
  void clearHistory();

public slots:
  void done(int r);
};

#endif
