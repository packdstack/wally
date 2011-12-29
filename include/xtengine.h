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

#ifndef XTENGINE_H
#define XTENGINE_H

#include <QtCore>
#include <QtGui>

#include "engine.h"
#include "gui.h"

namespace XtEngine
{
  class Item : public Engine::Item
  {
    Q_OBJECT

  public:
    virtual ~Item() { }

    virtual bool isValidItem() const = 0;
  };

  class Core : public Engine::Core
  {
    Q_OBJECT

  protected:
    int currentItemIndex;

  public:
    Core(const QString &tempStorageDir, QObject *parent = 0) :
      Engine::Core(tempStorageDir,parent), currentItemIndex(-1) { }
    virtual ~Core() { }
  };

  class DialogWidget : public QWidget
  {
    Q_OBJECT

    Core *_core;

  public:
    DialogWidget(Core *core, QWidget *parent = 0) : QWidget(parent), _core(core) { }
    virtual ~DialogWidget() { }

    Engine::Core *core() const { return _core; }

    virtual Item *returnItem() = 0;
    virtual void setupFromItem(Item *item = 0) = 0;
    virtual bool validateInput() { return true; }
  };

  class Dialog : public Gui::Dialog
  {
    Q_OBJECT

    Core *_core;

    QDialogButtonBox *dialogButtons;
    DialogWidget *dialogWidget;
    virtual DialogWidget *newDialogWidget(Core *core) = 0;

  private slots:
    void dialogButtonClicked(QAbstractButton *button);

  public:
    Dialog(Core *core, QWidget *parent = 0) :
      Gui::Dialog(Gui::Dialog::CenterOfScreen,parent,Qt::WindowSystemMenuHint),
      _core(core) { }
    virtual ~Dialog() { }

    Item *execute(Item *item = 0);
  };

  class ItemDelegate : public QItemDelegate
  {
    Q_OBJECT

    QLabel *label;

  public:
    ItemDelegate(QObject *parent = 0);
    virtual ~ItemDelegate();

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  };

  class SettingsWidget : public Engine::SettingsWidget
  {
    Q_OBJECT

  protected:
    virtual Dialog *newDialog() = 0;

  public:
    SettingsWidget(Engine::Core *core, QWidget *parent = 0) : Engine::SettingsWidget(core,parent) { }
    virtual ~SettingsWidget() { }
  };
}

#endif
