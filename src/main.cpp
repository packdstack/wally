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

#ifdef WALLY_STATIC
  #include <QtPlugin>

  Q_IMPORT_PLUGIN(qgif)
  Q_IMPORT_PLUGIN(qico)
  Q_IMPORT_PLUGIN(qjpeg)
  Q_IMPORT_PLUGIN(qmng)
  Q_IMPORT_PLUGIN(qsvg)
  Q_IMPORT_PLUGIN(qtiff)
  Q_IMPORT_PLUGIN(qsqlite)
#endif

#include <QTimer>
#include <QMessageBox>

#include "splash.h"
#include "files.h"
#include "folders.h"
#include "flickr.h"
#include "yahoo.h"
#include "panoramio.h"
#include "pikeo.h"
#include "ipernity.h"
#include "photobucket.h"
#include "buzznet.h"
#include "picasa.h"
#include "smugmug.h"
#include "bing.h"
#include "google.h"
#include "vladstudio.h"
#include "deviantart.h"
#include "wally.h"
#include "defs.h"

int main(int argc, char **argv)
{
  int err = 0;
  Wally::Application *wally = new Wally::Application(argc,argv);

  wally->loadLanguages();

  if (!wally->isCommandLineHelp())
  {
    if (!wally->isSingleInstance())
    {
      QMessageBox::critical(0,QCoreApplication::translate("Main","Error"),
                            QCoreApplication::translate("Main","Another instance of") +
                            " " + APPLICATION_NAME + " " +
                            QCoreApplication::translate("Main","is already running"));
    }
    else
    {
      bool upgraded = false;
      QMessageBox::StandardButton selectedButton = QMessageBox::Yes;

      if (wally->currentVersion() < Version(APPLICATION_VERSION))
      {
        selectedButton = QMessageBox::warning(0,QCoreApplication::translate("Main","Disclaimer"),
                                                QCoreApplication::translate(
                                                  "Main","<b>The Author takes no responsibility over the content " \
                                                         "that Wally downloads from photo sharing web sites.<br>" \
                                                         "The Author is in no way responsible for any such content.</b><br><br>" \
                                                         "If this is a problem for you, please use only local engines like " \
                                                         "\"Files\" or local \"Folders\".<br>" \
                                                         "Otherwise, please select <b>\"No\"</b> to exit Wally, " \
                                                         "or use it <b>at your own risk.</b><br><br>" \
                                                         "(If you proceed, this message will appear only once)<br><br>" \
                                                         "Do you accept the above condition?"),
                                              QMessageBox::Yes | QMessageBox::No,QMessageBox::No);
        upgraded = true;
      }

      if (selectedButton == QMessageBox::Yes)
      {
        wally->initialize();

        QSettings sourceSettings;

        sourceSettings.beginGroup(MAIN_SECTION);
        bool splashScreenDisabled = sourceSettings.value(DISABLE_SPLASH_SCREEN,false).toBool();

        SplashScreen *splash = 0;
        if (!splashScreenDisabled)
        {
          splash = new SplashScreen;
          splash->show();
        }

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading Files module ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->addEngine(new Files::Core(wally->tempStorageDir()));
        if (!splashScreenDisabled)
          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading Folders module ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->addEngine(new Folders::Core(wally->tempStorageDir()));
        if (!splashScreenDisabled)
          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading Flickr module ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->addEngine(new Flickr::Core(wally->tempStorageDir()));
        if (!splashScreenDisabled)
          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading Yahoo! module ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->addEngine(new Yahoo::Core(wally->tempStorageDir()));
        if (!splashScreenDisabled)
          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading Panoramio module ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->addEngine(new Panoramio::Core(wally->tempStorageDir()));
        if (!splashScreenDisabled)
          splash->repaint();

//        if (!splashScreenDisabled)
//          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading Pikeo module ..."),
//                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
//        wally->addEngine(new Pikeo::Core(wally->tempStorageDir()));
//        if (!splashScreenDisabled)
//          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading Ipernity module ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->addEngine(new Ipernity::Core(wally->tempStorageDir()));
        if (!splashScreenDisabled)
          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading Photobucket module ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->addEngine(new Photobucket::Core(wally->tempStorageDir()));
        if (!splashScreenDisabled)
          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading Buzznet module ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->addEngine(new Buzznet::Core(wally->tempStorageDir()));
        if (!splashScreenDisabled)
          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading Picasa module ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->addEngine(new Picasa::Core(wally->tempStorageDir()));
        if (!splashScreenDisabled)
          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading SmugMug module ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->addEngine(new SmugMug::Core(wally->tempStorageDir()));
        if (!splashScreenDisabled)
          splash->repaint();

//        if (!splashScreenDisabled)
//          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading Bing module ..."),
//                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
//        wally->addEngine(new Bing::Core(wally->tempStorageDir()));
//        if (!splashScreenDisabled)
//          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading Google module ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->addEngine(new Google::Core(wally->tempStorageDir()));
        if (!splashScreenDisabled)
          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading Vladstudio module ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->addEngine(new Vladstudio::Core(wally->tempStorageDir()));
        if (!splashScreenDisabled)
          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading deviantART module ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->addEngine(new DeviantArt::Core(wally->tempStorageDir()));
        if (!splashScreenDisabled)
          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Loading settings ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->loadSettings();
        if (!splashScreenDisabled)
          splash->repaint();

        if (!splashScreenDisabled)
          splash->showMessage(QCoreApplication::translate("SplashScreen","Launching Wally ..."),
                              Qt::AlignRight | Qt::AlignBottom,Qt::white);
        wally->setupBeforeLaunch();
        if (!splashScreenDisabled)
          splash->repaint();

        if (!splashScreenDisabled)
          QTimer::singleShot(3000,splash,SLOT(deleteLater()));
        if (upgraded)
          QTimer::singleShot(6000,wally,SLOT(showTrayMessage()));

        err = wally->exec();
      }
    }
  }

  delete wally;

  return err;
}
