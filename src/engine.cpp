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

#include "engine.h"

using namespace Engine;

Core::Core(const QString &tempStorageDir, QObject *parent) : QAbstractItemModel(parent)
{
  _active = true;
  _tempStorageDir = tempStorageDir;
  _selectionModel = new QItemSelectionModel(this);
}

Core::~Core()
{
  qDeleteAll(list);
}

QModelIndex Core::index(int row, int column, const QModelIndex & /* parent */) const
{
  if ((rowCount() > 0) && (columnCount() > 0) &&
      (row >= 0) && (column >= 0) &&
      (row < rowCount()) && (column < columnCount()))
    return createIndex(row,column);
  else
    return QModelIndex();
}

QVariant Core::data(const QModelIndex &index, int role) const
{
  Item *item;
  QVariant var;

  if (!index.isValid() || !list.at(index.row()))
    return QVariant();

  item = list.at(index.row());

  switch (role)
  {
    case Qt::UserRole:
      var.setValue(item);
      return var;

    case Qt::DisplayRole:
      return item->data();

    case Qt::TextAlignmentRole:
      return int(Qt::AlignLeft | Qt::AlignVCenter);

    case Qt::SizeHintRole:
      QTextDocument document;
      document.setHtml(item->data().toString());
      return document.size().toSize() + QSize(2*ITEM_VIEW_MARGIN,2*ITEM_VIEW_MARGIN);
  }

  return QVariant();
}

bool Core::insertRows(int row, int count, const QModelIndex &parent)
{
  int i;

  if (row == -1)
    row = rowCount();

  qDebug() << name().toAscii().constData() << "::Core::insertRows(" << row << "," << count << ")";

  beginInsertRows(parent,row,row + count - 1);

  for (i = 0; i < count; i++)
    list.insert(row + i,0);

  endInsertRows();

  return true;
}

bool Core::removeRows(const QItemSelectionModel *selectionModel)
{
  int i;
  bool ok = true;

  for (i = selectionModel->selectedRows().size() - 1; ok && (i >= 0); i--)
    ok &= removeRows(selectionModel->selectedRows().at(i).row(),1);

  return ok;
}

bool Core::removeRows(int row, int count, const QModelIndex &parent)
{
  qDebug() << name().toAscii().constData() << "::Core::removeRows(" << row << "," << count << ")";

  beginRemoveRows(parent,row,row + count - 1);

  while (count--)
    delete list.takeAt(row + count);

  endRemoveRows();

  return true;
}

bool Core::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (!index.isValid())
    return false;

  if ((role == Qt::EditRole) || (role == Qt::UserRole))
  {
    qDebug() << name().toAscii().constData() << "::Core::setData(" << index.row() << ")";

    Item *item = value.value<Item *>();

    if (list.at(index.row()) != item)
      delete list.at(index.row());

    list[index.row()] = item;

    emit dataChanged(index,index);

    return true;
  }

  return false;
}

void Core::edit()
{
  qDebug() << name().toAscii().constData() << "::Core::edit()";

  QListIterator<Engine::Item *> item(sourceList);

  list.clear();

  while (item.hasNext())
    list << newItem(item.next());
}

void Core::revert()
{
  qDebug() << name().toAscii().constData() << "::Core::revert()";

  qDeleteAll(list);
  edit();

  doRevert();

  QAbstractItemModel::revert();
  selectionModel()->clearSelection();
  reset();
}

bool Core::submit()
{
  qDebug() << name().toAscii().constData() << "::Core::submit()";

  qDeleteAll(sourceList);
  sourceList = list;

  doSubmit();

  QAbstractItemModel::submit();
  return true;
}

Core *Core::operator<<(Item *item)
{
  bool found = false;
  QVariant var;
  QListIterator<Item *> sourceItem(list);

  while (sourceItem.hasNext() && !found)
    found = sourceItem.next()->equalTo(item);

  if (!found)
  {
    insertRows(-1,1,QModelIndex());
    var.setValue(item);
    setData(index(rowCount()-1,0),var,Qt::UserRole);
  }

  return this;
}

void Core::saveState(QSettings &settings) const
{
  int i = 0;
  QListIterator<Item *> item(list);

  qDebug() << name().toAscii().constData() << "::Core::saveState(...)";

  while (item.hasNext())
    item.next()->saveState(settings,name(),i++);
}

void Core::loadSettings(QSettings &settings)
{
  QStringListIterator group(settings.childGroups());
  Item *item;

  qDebug() << name().toAscii().constData() << "::Core::loadSettings(...)";

  _active = settings.value(ENGINE_ACTIVE,true).toBool();

  while (group.hasNext())
  {
    item = newItem();
    item->loadSettings(settings,group.peekNext());
    item->loadState(settings,group.peekNext());
    group.next();
    list << item;
  }

  submit();
}

void Core::saveSettings(QSettings &settings) const
{
  int i = 0;
  QListIterator<Item *> item(list);

  qDebug() << name().toAscii().constData() << "::Core::saveSettings(...)";

  settings.setValue(ENGINE_ACTIVE,_active);

  while (item.hasNext())
    item.next()->saveSettings(settings,name(),i++);
}

SettingsWidget::SettingsWidget(Core *core, QWidget *parent) : QWidget(parent), _core(core)
{
  core->selectionModel()->clearSelection();
}
