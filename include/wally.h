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

#ifndef WALLY_H
#define WALLY_H

#include <QtCore>
#include <QtGui>
#include <QtSql>

#include "defs.h"
#include "engine.h"
#include "version.h"

#define DATABASE_CURRENT_VERSION 1
#define DEFAULT_INTERVAL 2
#define APPLICATION_NAME "Wally"
#define APPLICATION_VERSION "2.4.5"
#define RUN_PATH "run"
#define PID_FILENAME "wally.pid"
#define MAX_SIZE_FAILURES 20
#define TIMEOUT 30000

#if defined(Q_WS_WIN) || defined(Q_WS_PM)
  #include <fstream>
#endif

#ifdef Q_WS_WIN
  #include <windows.h>
#endif

namespace Wally
{
  class Application : public QApplication
  {
    Q_OBJECT

  public:
    enum Position { Centered = 0, Tiled = 1, CenterTiled = 2, CenteredMaxpect = 3,
                    TiledMaxpect = 4, Scaled = 5, CenteredAutoFit = 6, ScaleAndCrop = 7,
                    SymmetricalTiled = 8, MirroredTiled = 9, SymmetricalMirroredTiled = 10 };

  private:
    #ifdef Q_WS_X11
      enum WindowManager { wmUnknown, wmKDE3, wmKDE4, wmGnome, wmXfce, wmFluxbox,
                           wmBlackbox, wmWindowMaker, wmFVWM, wmGnomeShell };
    #endif

    enum States { sIdle, sSelectNextEngineFirst, sInitEngine, sSearchRequest,
                  sSelectNextEngineNext, sSizeRequest, sDownload, sInfoRequest, sCancel};
    enum Events { eOk, eNoEngines, eSearchNoData, eWrongSize, eCancel,
                  ePlayTimeout, eDownloadFailed, eDownloadOkButWrongSize };

    bool playTimerWasActive;
    bool playWasActive;
    bool singleInstance;
    bool commandLineHelp;
    bool captureKeymaps;
    bool dontSwitchState;

  #if defined(Q_WS_WIN) || defined(Q_WS_PM)
    std::ofstream fout;
    bool adWarningFired;
  #endif

  #ifdef Q_WS_X11
    WindowManager windowManager;
    void autoDetectWM();
  #endif

    States currentState;
    QLocale currentLocale;
    int sizeFailures;
    void switchToNextState(Events event);

    QTimer *timer;

    QString _tempStorageDir;
    int markEngineIndex;
    int currentEngineIndex;
    QList<Engine::Core *> engines;
    void selectEngine(int index);

    QSystemTrayIcon *trayIcon;
    QAction *savePhotoAction;
    QAction *playAction;
    QAction *cancelAction;
    QAction *nextPhotoAction;
    QAction *configureAction;
    QAction *aboutAction;
    QAction *aboutQtAction;
    QAction *historyAction;
    QAction *viewSourceAction;
    QAction *quitAction;
    QAction *getExifInfoAction;
    QAction *italianLanguageAction;
    QAction *englishLanguageAction;
    QAction *spanishLanguageAction;
    QAction *germanLanguageAction;
    QAction *frenchLanguageAction;
    QAction *russianLanguageAction;
    QAction *brazilianPortugueseLanguageAction;
    QAction *czechLanguageAction;
    QAction *polishLanguageAction;
    QAction *chineseLanguageAction;
    QAction *mandarinLanguageAction;
    QAction *catalanLanguageAction;
    QAction *greekLanguageAction;
    QAction *koreanLanguageAction;
    QAction *hungarianLanguageAction;
    QAction *danishLanguageAction;
    QAction *swedishLanguageAction;
    QAction *turkishLanguageAction;
    QWidget *hiddenWidgetForLanguagesActions;
    QList<QAction *> languagesActions;
    QSignalMapper *languagesSignalMapper;
    QMenu *contextMenu;
    QMenu *cancelContextMenu;
    QMenu *languagesMenu;
    QMap<QString, QTranslator *> translators;

    QSqlDatabase db;
    void initializeDB();
    void updateDBVersion();
    void updateDB();
    void cleanHistory(int now, int timeBack, int timeBackFactor);

    QSize currentPhotoSize;
    QFileInfo currentFile;
    PhotoInfo currentPhotoInfo;
    bool isSizeValid(const QSize &size) const;
    static bool isCenterStored(const QPoint &center, const QList< QPair< QPoint,QPair<bool,bool> > > &centers);
    bool isCenterInsideDesktop(const QPoint &center, const QSize &pixmapSize, const QSize &desktopSize);
    void recFillCentersList(const QSize &desktopSize, const QSize &pixmapSize,
                            QList< QPair< QPoint,QPair<bool,bool> > > &centers, const QPoint &center,
                            bool horizontalFlip, bool verticalFlip);
    void fillCentersList(const QSize &desktopSize, QSize pixmapSize,
                         QList< QPair< QPoint,QPair<bool,bool> > > &centers);
    QFileInfo adaptPhoto(const QFileInfo &file, const QString &format, const PhotoInfo &info);
    void tryCompletePhotoInfo();

    WallySettings settings;
    void saveEnginesState();
    void saveSettings();
    void applySettings(WallySettings newSettings);

    void checkPlayEnableable();
    bool isPIDBelongingToWally(int pid) const;
    void checkSingleInstance();

    void retranslateMenu();

    void drawTextWithShadowOnScene(const QString &text, QGraphicsScene *scene,
                                  Qt::AlignmentFlag alignment, int &y, bool isTitle = false);

    static void printDebugMsg(QtMsgType type, const char *msg);
    static bool actionCaseSensitiveLessThan(QAction *a1, QAction *a2);

  private slots:
    void start();
    void setupActions();
    void setupTrayIcon();
    void setupMenu();
    void initCompleted();
    void processSearchResult(bool hasData);
    void processPhotoSize(const QSize &size);
    void processDownloadedPhoto(bool ok, const QFileInfo &localFile);
    void processCollectedInfo(const PhotoInfo &info);
    void switchToIdle();
    void execTrayAction(QSystemTrayIcon::ActivationReason reason);

    void execPlay();
    void execPause();
    void execAbout();
    void execAboutQt();
    void execSavePhoto();
    void execShowSettings();
    void execCancel();
    void execNextPhoto();
    void execExploreImageSource();
    void execShowHistory();
    void execGetExifInfo();

    void changeLanguage(const QString &newLocaleName);

    void showPhotoOnScreen();
    void showPhotoOnScreen(const QString &engineName);

  public:
    Application(int &argc, char **argv);
    virtual ~Application();

    bool isSingleInstance() const { return singleInstance; }
    bool isCommandLineHelp() const { return commandLineHelp; }

    QString tempStorageDir() const { return _tempStorageDir; }

    void initialize();
    void loadLanguages();

    void addEngine(Engine::Core *e);

    void setupBeforeLaunch();

    void loadSettings();

    Version currentVersion() const;

  #ifdef Q_WS_X11
    bool x11EventFilter(XEvent *event);
  #endif

  #ifdef Q_WS_WIN
    bool winEventFilter(MSG *msg, long *result);
  #endif

  signals:
    void init();
    void search(bool randomMode);
    void sizeLookup();
    void download();
    void infoCollect();
    void cancel();

  public slots:
    void deActivate();
    void reActivate();
    void showTrayMessage();
    void clearHistory();
  };
}

#endif
