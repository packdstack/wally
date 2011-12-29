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

#include "utils.h"
#include "gui.h"
#include "about.h"

AboutDialog::AboutDialog(QWidget *parent) : Gui::Dialog(Gui::Dialog::TopOfScreen,parent)
{
  int row = 0;
  QString format, details;
  QMap<QString, QString> formats = getFormats();
  QStringList supportedFormats = getSupportedImageFormats();
  QStringListIterator supportedFormat(supportedFormats);

  setupUi(this);
  lblTitle->setText(qApp->applicationName() + " " + qApp->applicationVersion());
  wdgDetails->setVisible(false);

  twSupportedImageFormats->setRowCount(supportedFormats.size());
  while (supportedFormat.hasNext())
  {
    format = supportedFormat.next();

    twSupportedImageFormats->setItem(row,0,new QTableWidgetItem(format.toUpper()));
    twSupportedImageFormats->setItem(row++,1,new QTableWidgetItem(formats.value(format)));
  }

  twAdditionalInfo->setColumnCount(2);
  twAdditionalInfo->setRowCount(2);
  twAdditionalInfo->setItem(0,0,new QTableWidgetItem(tr("Compiled with:")));
  twAdditionalInfo->setItem(0,1,new QTableWidgetItem(QT_VERSION_STR));
  twAdditionalInfo->setItem(1,0,new QTableWidgetItem(tr("EXIF library:")));

#ifdef EXIF
  twAdditionalInfo->setItem(1,1,new QTableWidgetItem(tr("present")));
#else
  twAdditionalInfo->setItem(1,1,new QTableWidgetItem(tr("not present")));
#endif
  twTranslators->setColumnCount(2);
  twTranslators->setRowCount(17);
  twTranslators->setItem(0,0,new QTableWidgetItem(QIcon(":/images/uk"),tr("English")));
  twTranslators->setItem(0,1,new QTableWidgetItem("Antonio Di Monaco"));
  twTranslators->setItem(1,0,new QTableWidgetItem(QIcon(":/images/italy"),tr("Italian")));
  twTranslators->setItem(1,1,new QTableWidgetItem("Antonio Di Monaco"));
  twTranslators->setItem(2,0,new QTableWidgetItem(QIcon(":/images/russia"),tr("Russian")));
  twTranslators->setItem(2,1,new QTableWidgetItem(QString::fromUtf8("\320\242\320\260\321\202\321\214\321\217\320\275\320\260 \320\240\321\203\320\261\320\270\320\275\320\260")));
  twTranslators->setItem(3,0,new QTableWidgetItem(QIcon(":/images/spain"),tr("Spanish")));
  twTranslators->setItem(3,1,new QTableWidgetItem("Martino Vedana"));
  twTranslators->setItem(4,0,new QTableWidgetItem(QIcon(":/images/brazil"),tr("Portuguese (Brazil)")));
  twTranslators->setItem(4,1,new QTableWidgetItem(QString::fromUtf8("\115\303\241\162\143\151\157\40\115\157\162\141\145\163")));
  twTranslators->setItem(5,0,new QTableWidgetItem(QIcon(":/images/germany"),tr("German")));
  twTranslators->setItem(5,1,new QTableWidgetItem("Klaus-Peter Gores"));
  twTranslators->setItem(6,0,new QTableWidgetItem(QIcon(":/images/france"),tr("French")));
  twTranslators->setItem(6,1,new QTableWidgetItem("Nicolas Remy"));
  twTranslators->setItem(7,0,new QTableWidgetItem(QIcon(":/images/czech_republic"),tr("Czech")));
  twTranslators->setItem(7,1,new QTableWidgetItem(QString::fromUtf8("\115\141\162\164\151\156\40\120\141\166\154\303\255\153")));
  twTranslators->setItem(8,0,new QTableWidgetItem(QIcon(":/images/china"),tr("Chinese")));
  twTranslators->setItem(8,1,new QTableWidgetItem(QString::fromUtf8("\345\276\241\347\224\250\350\275\257\344\273\266")));
  twTranslators->setItem(9,0,new QTableWidgetItem(QIcon(":/images/poland"),tr("Polish")));
  twTranslators->setItem(9,1,new QTableWidgetItem("Dominik Szczerba"));
  twTranslators->setItem(10,0,new QTableWidgetItem(QIcon(":/images/catalonia"),tr("Catalan")));
  twTranslators->setItem(10,1,new QTableWidgetItem(QString::fromUtf8("\115\151\161\165\145\154\40\122\303\240\155\151\141")));
  twTranslators->setItem(11,0,new QTableWidgetItem(QIcon(":/images/greece"),tr("Greek")));
  twTranslators->setItem(11,1,new QTableWidgetItem(QString::fromUtf8("\316\230\317\211\316\274\316\254\317\202\40\316\244\317\203\316\254\316\262\316\261\316\273\316\277\317\202")));
  twTranslators->setItem(12,0,new QTableWidgetItem(QIcon(":/images/korea"),tr("Korean")));
  twTranslators->setItem(12,1,new QTableWidgetItem(QString::fromUtf8("\354\235\264\353\215\225\355\235\254")));
  twTranslators->setItem(13,0,new QTableWidgetItem(QIcon(":/images/hungary"),tr("Hungarian")));
  twTranslators->setItem(13,1,new QTableWidgetItem(QString::fromUtf8("\122\165\155\303\241\156\40\123\303\241\156\144\157\162")));
  twTranslators->setItem(14,0,new QTableWidgetItem(QIcon(":/images/denmark"),tr("Danish")));
  twTranslators->setItem(14,1,new QTableWidgetItem("Michael Borries"));
  twTranslators->setItem(15,0,new QTableWidgetItem(QIcon(":/images/sweden"),tr("Swedish")));
  twTranslators->setItem(15,1,new QTableWidgetItem("Andreas Klintemyr"));
  twTranslators->setItem(16,0,new QTableWidgetItem(QIcon(":/images/turkey"),tr("Turkish")));
  twTranslators->setItem(16,1,new QTableWidgetItem(QString::fromUtf8("\122\145\143\145\160\40\303\207\141\153\141\156")));
  twTranslators->sortItems(0);

  twSupportedImageFormats->resizeColumnsToContents();
  twSupportedImageFormats->resizeRowsToContents();
  twAdditionalInfo->resizeColumnsToContents();
  twAdditionalInfo->resizeRowsToContents();
  twTranslators->resizeColumnsToContents();
  twTranslators->resizeRowsToContents();

  connect(pbDetails,SIGNAL(toggled(bool)),wdgDetails,SLOT(setVisible(bool)));

  lblLogo->setPixmap(QPixmap(":/images/logo").scaled(lblLogo->size(),
                                                     Qt::KeepAspectRatio,
                                                     Qt::SmoothTransformation));
}
