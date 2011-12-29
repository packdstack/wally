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

#include "engine.h"
#include "xtengine.h"

using namespace XtEngine;

Item *Dialog::execute(Item *item)
{
  Item *newItem = 0;
  QVBoxLayout *mainLayout = new QVBoxLayout;
  dialogButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                       Qt::Horizontal,this);

  dialogWidget = newDialogWidget(_core);
  dialogWidget->setupFromItem(item);
  dialogWidget->layout()->setContentsMargins(0,0,0,0);

  mainLayout->addWidget(dialogWidget);
  mainLayout->addStretch();
  mainLayout->addWidget(dialogButtons);
  mainLayout->setSizeConstraint(QLayout::SetFixedSize);

  setLayout(mainLayout);

  setWindowTitle(dialogWidget->windowTitle());

  connect(dialogButtons,SIGNAL(clicked(QAbstractButton *)),
          this,SLOT(dialogButtonClicked(QAbstractButton *)));

  if (exec() == Dialog::Accepted)
    newItem = dialogWidget->returnItem();

  deleteLater();

  return newItem;
}

void Dialog::dialogButtonClicked(QAbstractButton *button)
{
  switch (dialogButtons->buttonRole(button))
  {
    case QDialogButtonBox::AcceptRole:
      if (dialogWidget->validateInput())
        accept();
      break;

    case QDialogButtonBox::RejectRole:
      reject();
      break;

    default:
      break;
  }
}

ItemDelegate::ItemDelegate(QObject *parent) : QItemDelegate(parent)
{
  label = new QLabel;
  label->setAutoFillBackground(true);
  label->setTextFormat(Qt::RichText);
  label->setMargin(ITEM_VIEW_MARGIN);
}

ItemDelegate::~ItemDelegate()
{
  delete label;
}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
  if (index.isValid())
  {
    Engine::Item *item = index.model()->data(index,Qt::UserRole).value<Engine::Item *>();
    label->setBackgroundRole((option.state & QStyle::State_Selected)? QPalette::Highlight :
                             ((!(index.row() % 2))? QPalette::Base : QPalette::AlternateBase));
    label->setForegroundRole((option.state & QStyle::State_Selected)? QPalette::HighlightedText :
                                                                      QPalette::Text);
    label->setFixedSize(option.rect.width(),option.rect.height());
    label->setText(item->data().toString());

    painter->setRenderHint(QPainter::Antialiasing,true);
    painter->drawPixmap(option.rect,QPixmap::grabWidget(label));
  }
}
