/*
TRANSLATOR Wally::Application
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

#include <iostream>
#include <QtAlgorithms>
#include <QtNetwork>
#include <QtDebug>

#include "exif.h"
#include "engine.h"
#include "httpengine.h"
#include "settings.h"
#include "about.h"
#include "history.h"
#include "wally.h"

#ifdef Q_WS_X11
  #include <QX11Info>
  #include <X11/Xlib.h>
  #include <X11/keysym.h>
  #include <QtDBus>
#endif

#ifdef Q_WS_WIN
  #include <fstream>
  #include <windows.h>
  #include <wininet.h>
  #include <shlobj.h>
#endif

#ifdef Q_WS_MAC
  #include <strings.h>
  #include <stdlib.h>
  #include <sys/sysctl.h>
#endif

#ifdef Q_WS_PM
  #define INCL_WINWORKPLACE
  #include <os2.h>
  #include <os2emx.h>
#endif

static const char *statesStrs[] = { "sIdle", "sSelectNextEngineFirst", "sInitEngine",
                                    "sSearchRequest", "sSelectNextEngineNext", "sSizeRequest",
                                    "sDownload", "sInfoRequest", "sCancel", "sAbort" };
static const char *eventsStrs[] = { "eOk", "eNoEngines", "eSearchNoData", "eWrongSize",
                                    "eCancel", "ePlayTimeout", "eDownloadFailed",
                                    "eDownloadOkButWrongSize" };

static bool debugMode;
static bool httpDebugMode;

using namespace Wally;

Application::Application(int &argc, char **argv) : QApplication(argc,argv)
{
  QStringList args = arguments();
  debugMode = args.contains("-debug",Qt::CaseInsensitive);
  httpDebugMode = args.contains("-debughttp",Qt::CaseInsensitive);
  languagesMenu = contextMenu = cancelContextMenu = 0;
  dontSwitchState = singleInstance = commandLineHelp = playWasActive = playTimerWasActive = false;
  hiddenWidgetForLanguagesActions = 0;
  playAction = 0;

  qInstallMsgHandler(printDebugMsg);

  if (debugMode)
  {
  #if defined(Q_WS_WIN) || defined(Q_WS_PM)
    fout.open("WallyDebug.txt");
    std::cout.rdbuf(fout.rdbuf());
  #endif

    qDebug() << "Application::Application()";
  }

  #ifdef Q_WS_WIN
    adWarningFired = false;
  #endif

  #ifdef Q_WS_X11
  int forceIdx;
  windowManager = wmUnknown;
  if (((forceIdx = args.indexOf("-forcewm",Qt::CaseInsensitive)) != -1) &&
      (forceIdx < (args.size() - 1)))
  {
    QString wmStr = args.at(forceIdx+1);
    if (!wmStr.compare("KDE3",Qt::CaseInsensitive))
      windowManager = wmKDE3;
    else if (!wmStr.compare("KDE4",Qt::CaseInsensitive))
      windowManager = wmKDE4;
    else if (!wmStr.compare("Gnome",Qt::CaseInsensitive))
      windowManager = wmGnome;
    else if (!wmStr.compare("XFCE",Qt::CaseInsensitive))
      windowManager = wmXfce;
    else if (!wmStr.compare("FluxBox",Qt::CaseInsensitive))
      windowManager = wmFluxbox;
    else if (!wmStr.compare("FVWM",Qt::CaseInsensitive))
      windowManager = wmFVWM;
    else if (!wmStr.compare("BlackBox",Qt::CaseInsensitive))
      windowManager = wmBlackbox;
    else if (!wmStr.compare("WindowMaker",Qt::CaseInsensitive))
      windowManager = wmWindowMaker;
    else if (!wmStr.compare("GnomeShell",Qt::CaseInsensitive))
      windowManager = wmGnomeShell;
  }
  #endif

  if ((commandLineHelp = (args.indexOf("-help") != -1)))
  {
    QByteArray help = QByteArray("Usage:\n\t-debug\t\tShows debug output") +
                      "\n\t-debughttp\tShows verbose HTTP output" +
                      "\n\t-forcewm <wm>\t" +
                      "Use specific window manager (Linux only)\n\n\t" +
                      "where <wm> can be:\tKDE3, KDE4, Gnome, XFCE, FluxBox,\n" +
                      "\t\t\t\tFVWM, BlackBox, WindowMaker, GnomeShell";

    std::cout << APPLICATION_NAME << " " << APPLICATION_VERSION << std::endl << std::endl;
    std::cout << help.constData() << std::endl;
  }
  else
  {
    checkSingleInstance();

    if (isSingleInstance())
    {
      setApplicationName(APPLICATION_NAME);
      setApplicationVersion(APPLICATION_VERSION);
      setOrganizationName("BeCrux");
    }
  }
}

Application::~Application()
{
  qDebug() << "Application::~Application()";

  db.close();

  delete languagesMenu;
  delete contextMenu;
  delete cancelContextMenu;

  delete hiddenWidgetForLanguagesActions;

  if (engines.size())
    qDeleteAll(engines);

#if !defined(Q_WS_WIN) && !defined(Q_WS_PM)
  QFile::remove(_tempStorageDir + "/" + RUN_PATH + "/" + PID_FILENAME);
#endif

#if defined(Q_WS_WIN) || defined(Q_WS_PM)
  if (debugMode)
    fout.close();
#endif
}

void Application::loadLanguages()
{
  QSettings settings;
  QTranslator *translator;

  translator = new QTranslator(this);
  translator->load(":/languages/italian");
  translators[QLocale(QLocale::Italian).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/spanish");
  translators[QLocale(QLocale::Spanish).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/german");
  translators[QLocale(QLocale::German).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/french");
  translators[QLocale(QLocale::French).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/russian");
  translators[QLocale(QLocale::Russian).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/brazilian_portuguese");
  translators[QLocale(QLocale::Portuguese,QLocale::Brazil).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/czech");
  translators[QLocale(QLocale::Czech,QLocale::CzechRepublic).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/polish");
  translators[QLocale(QLocale::Polish,QLocale::Poland).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/chinese");
  translators[QLocale(QLocale::Chinese,QLocale::China).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/mandarin");
  translators[QLocale(QLocale::Vietnamese,QLocale::China).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/catalan");
  translators[QLocale(QLocale::Catalan,QLocale::Andorra).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/greek");
  translators[QLocale(QLocale::Greek,QLocale::Greece).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/korean");
  translators[QLocale(QLocale::Korean,QLocale::RepublicOfKorea).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/hungarian");
  translators[QLocale(QLocale::Hungarian,QLocale::Hungary).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/danish");
  translators[QLocale(QLocale::Danish,QLocale::Denmark).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/swedish");
  translators[QLocale(QLocale::Swedish,QLocale::Sweden).name()] = translator;

  translator = new QTranslator(this);
  translator->load(":/languages/turkish");
  translators[QLocale(QLocale::Turkish,QLocale::Turkey).name()] = translator;

  currentLocale = QLocale::English;
  settings.beginGroup(MAIN_SECTION);
  changeLanguage(settings.value(LOCALE,QLocale::system().name()).toString());
  settings.endGroup();
}

void Application::initialize()
{
  setupTrayIcon();
  setupActions();
  setupMenu();
  setQuitOnLastWindowClosed(false);
  qsrand(QDateTime::currentDateTime().toTime_t());

#ifdef Q_WS_X11
  QDir(QDir::homePath()).mkdir(TEMP_PATH);
  _tempStorageDir = QDir::homePath() + "/" + TEMP_PATH;
#else
  QDir::home().root().mkpath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
  _tempStorageDir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif

  currentState = sIdle;

  currentEngineIndex = -1;

  timer = new QTimer(this);
  timer->setSingleShot(true);
  timer->setInterval(DEFAULT_INTERVAL * 60000);
  connect(timer,SIGNAL(timeout()),this,SLOT(start()));

  if ((captureKeymaps = !trayIcon->isSystemTrayAvailable()))
  {
    #ifdef Q_WS_X11
      XGrabKey(QX11Info::display(),AnyKey,AnyModifier,QX11Info::appRootWindow(),
              true,GrabModeSync,GrabModeSync);
    #endif
  }

  retranslateMenu();

  #ifdef Q_WS_X11
    autoDetectWM();
  #endif

  initializeDB();
}

void Application::printDebugMsg(QtMsgType type, const char *msg)
{
  switch (type)
  {
    case QtWarningMsg:
      if (httpDebugMode)
        std::cout << msg << std::endl << std::flush;
      break;

    default:
      if (debugMode)
        std::cout << msg << std::endl << std::flush;
      break;
  }
}

bool Application::isPIDBelongingToWally(int pid) const
{
#ifdef Q_WS_X11
  return QFileInfo(QString("/proc/") + QString::number(pid) +
                   "/exe").symLinkTarget().contains(APPLICATION_NAME,Qt::CaseInsensitive);
#endif

#ifdef Q_WS_MAC
  struct kinfo_proc *result, *ptr;
  int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
  size_t length, i;

  result = NULL;

  try
  {
    length = 0;
    if (sysctl((int *) name,(sizeof(name) / sizeof(*name)) - 1,NULL,&length,NULL,0) == -1)
      throw;

    if ((result = (kinfo_proc *) malloc(length)) == NULL)
      throw;

    if (sysctl((int *) name,(sizeof(name) / sizeof(*name)) - 1,result,&length,NULL,0) == -1)
      throw;

    for (i = 0, ptr = result;
         (i < (length / sizeof(kinfo_proc))) &&
         ((ptr->kp_proc.p_pid != pid) || strcasecmp(ptr->kp_proc.p_comm,APPLICATION_NAME));
         i++, ptr++) ;

    free(result);

    return (i < (length / sizeof(kinfo_proc)));
  }
  catch (...)
  {
    if (result != NULL)
      free(result);

    return true;
  }
#endif

#if defined(Q_WS_WIN) || defined(Q_WS_PM)
  Q_UNUSED(pid)

  return false;
#endif
}

void Application::checkSingleInstance()
{
#if defined(Q_WS_WIN) || defined(Q_WS_PM)

  #ifndef QT_NO_SHAREDMEMORY
    QSharedMemory *sharedMemory = new QSharedMemory(QString(APPLICATION_NAME) +
                                                    "SingleApplicationKey",this);
    singleInstance = sharedMemory->create(1);
  #else
    singleInstance = true;
  #endif

#else

  QString pidStr;
  QDir(QDir::homePath()).mkdir(TEMP_PATH);
  QDir(QDir::homePath() + "/" + TEMP_PATH).mkdir(RUN_PATH);
  QFile pidFile(QDir::homePath() + "/" + TEMP_PATH + "/" + RUN_PATH + "/" + PID_FILENAME);

  singleInstance = true;
  if (pidFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    pidStr = pidFile.readLine();
    pidFile.close();
    singleInstance = !isPIDBelongingToWally(pidStr.trimmed().toInt());
  }

  if (singleInstance)
  {
    pidFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    pidFile.write((QString::number(QCoreApplication::applicationPid()) + "\n").toAscii());
    pidFile.close();
  }

#endif

  if (!singleInstance)
    qDebug() << "Application::Application() found another instance running";
}

void Application::selectEngine(int index)
{
  qDebug() << "Application::selectEngine("
           << engines.at(index)->name().toAscii().constData() << ")";

  for (int i = 0; i < engines.size(); ++i)
  {
    Engine::Core *e = engines.at(i);

    disconnect(e,SIGNAL(initCompleted()),
                this,SLOT(initCompleted()));
    disconnect(e,SIGNAL(searchCompleted(bool)),
                this,SLOT(processSearchResult(bool)));
    disconnect(e,SIGNAL(sizeLookupCompleted(const QSize &)),
                this,SLOT(processPhotoSize(const QSize &)));
    disconnect(e,SIGNAL(downloadCompleted(bool,const QFileInfo &)),
                this,SLOT(processDownloadedPhoto(bool,const QFileInfo &)));
    disconnect(e,SIGNAL(infoCollectCompleted(const PhotoInfo &)),
                this,SLOT(processCollectedInfo(const PhotoInfo &)));
    disconnect(e,SIGNAL(cancelCompleted()),
                this,SLOT(switchToIdle()));

    disconnect(this,SIGNAL(init()),e,SLOT(init()));
    disconnect(this,SIGNAL(search(bool)),e,SLOT(search(bool)));
    disconnect(this,SIGNAL(sizeLookup()),e,SLOT(sizeLookup()));
    disconnect(this,SIGNAL(download()),e,SLOT(download()));
    disconnect(this,SIGNAL(infoCollect()),e,SLOT(infoCollect()));
    disconnect(this,SIGNAL(cancel()),e,SLOT(cancel()));

    if (i == index)
    {
      connect(e,SIGNAL(initCompleted()),
              this,SLOT(initCompleted()));
      connect(e,SIGNAL(searchCompleted(bool)),
              this,SLOT(processSearchResult(bool)));
      connect(e,SIGNAL(sizeLookupCompleted(const QSize &)),
              this,SLOT(processPhotoSize(const QSize &)));
      connect(e,SIGNAL(downloadCompleted(bool,const QFileInfo &)),
              this,SLOT(processDownloadedPhoto(bool,const QFileInfo &)));
      connect(e,SIGNAL(infoCollectCompleted(const PhotoInfo &)),
              this,SLOT(processCollectedInfo(const PhotoInfo &)));
      connect(e,SIGNAL(cancelCompleted()),
              this,SLOT(switchToIdle()));

      connect(this,SIGNAL(init()),e,SLOT(init()));
      connect(this,SIGNAL(search(bool)),e,SLOT(search(bool)));
      connect(this,SIGNAL(sizeLookup()),e,SLOT(sizeLookup()));
      connect(this,SIGNAL(download()),e,SLOT(download()));
      connect(this,SIGNAL(infoCollect()),e,SLOT(infoCollect()));
      connect(this,SIGNAL(cancel()),e,SLOT(cancel()));
    }
  }
}

void Application::setupActions()
{
  qDebug() << "Application::setupActions()";

  cancelAction = new QAction(QIcon(":/images/cancel"),QString(),this);
  connect(cancelAction,SIGNAL(triggered()),this,SLOT(execCancel()));

  playAction = new QAction(QIcon(":/images/control_play"),QString(),this);
  playAction->setData(false);
  connect(playAction,SIGNAL(triggered()),this,SLOT(execPlay()));

  nextPhotoAction = new QAction(QIcon(":/images/active"),QString(),this);
  connect(nextPhotoAction,SIGNAL(triggered()),this,SLOT(execNextPhoto()));
  nextPhotoAction->setEnabled(false);

  savePhotoAction = new QAction(QIcon(":images/photo_save"),QString(),this);
  connect(savePhotoAction,SIGNAL(triggered()),this,SLOT(execSavePhoto()));
  savePhotoAction->setEnabled(false);

  getExifInfoAction = new QAction(QIcon(":images/exif_info"),QString(),this);
  connect(getExifInfoAction,SIGNAL(triggered()),this,SLOT(execGetExifInfo()));
  getExifInfoAction->setEnabled(false);

  configureAction = new QAction(QIcon(":/images/configure"),QString(),this);
  connect(configureAction,SIGNAL(triggered()),this,SLOT(execShowSettings()));

  aboutAction = new QAction(QIcon(":/images/about"),QString(),this);
  connect(aboutAction,SIGNAL(triggered()),this,SLOT(execAbout()));

  aboutQtAction = new QAction(QIcon(":/images/qt"),QString(),this);
  connect(aboutQtAction,SIGNAL(triggered()),this,SLOT(execAboutQt()));

  historyAction = new QAction(QIcon(":/images/history"),QString(),this);
  connect(historyAction,SIGNAL(triggered()),qApp,SLOT(execShowHistory()));

  quitAction = new QAction(QIcon(":/images/quit"),QString(),this);
  connect(quitAction,SIGNAL(triggered()),qApp,SLOT(quit()));

  viewSourceAction = new QAction(QIcon(":/images/source_url"),QString(),this);
  connect(viewSourceAction,SIGNAL(triggered()),this,SLOT(execExploreImageSource()));

  languagesSignalMapper = new QSignalMapper(this);
  connect(languagesSignalMapper,SIGNAL(mapped(const QString &)),
          this,SLOT(changeLanguage(const QString &)));

  hiddenWidgetForLanguagesActions = new QWidget;
  hiddenWidgetForLanguagesActions->setVisible(false);

  englishLanguageAction = new QAction(QIcon(":/images/uk"),QString(),this);
  languagesActions << englishLanguageAction;
  languagesSignalMapper->setMapping(englishLanguageAction,QLocale(QLocale::English).name());
  connect(englishLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  italianLanguageAction = new QAction(QIcon(":/images/italy"),QString(),this);
  languagesActions << italianLanguageAction;
  languagesSignalMapper->setMapping(italianLanguageAction,QLocale(QLocale::Italian).name());
  connect(italianLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  spanishLanguageAction = new QAction(QIcon(":/images/spain"),QString(),this);
  languagesActions << spanishLanguageAction;
  languagesSignalMapper->setMapping(spanishLanguageAction,QLocale(QLocale::Spanish).name());
  connect(spanishLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  germanLanguageAction = new QAction(QIcon(":/images/germany"),QString(),this);
  languagesActions << germanLanguageAction;
  languagesSignalMapper->setMapping(germanLanguageAction,QLocale(QLocale::German).name());
  connect(germanLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  frenchLanguageAction = new QAction(QIcon(":/images/france"),QString(),this);
  languagesActions << frenchLanguageAction;
  languagesSignalMapper->setMapping(frenchLanguageAction,QLocale(QLocale::French).name());
  connect(frenchLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  russianLanguageAction = new QAction(QIcon(":/images/russia"),QString(),this);
  languagesActions << russianLanguageAction;
  languagesSignalMapper->setMapping(russianLanguageAction,QLocale(QLocale::Russian).name());
  connect(russianLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  brazilianPortugueseLanguageAction = new QAction(QIcon(":/images/brazil"),QString(),this);
  languagesActions << brazilianPortugueseLanguageAction;
  languagesSignalMapper->setMapping(brazilianPortugueseLanguageAction,
                                    QLocale(QLocale::Portuguese,QLocale::Brazil).name());
  connect(brazilianPortugueseLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  czechLanguageAction = new QAction(QIcon(":/images/czech_republic"),QString(),this);
  languagesActions << czechLanguageAction;
  languagesSignalMapper->setMapping(czechLanguageAction,
                                    QLocale(QLocale::Czech,QLocale::CzechRepublic).name());
  connect(czechLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  polishLanguageAction = new QAction(QIcon(":/images/poland"),QString(),this);
  languagesActions << polishLanguageAction;
  languagesSignalMapper->setMapping(polishLanguageAction,
                                    QLocale(QLocale::Polish,QLocale::Poland).name());
  connect(polishLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  chineseLanguageAction = new QAction(QIcon(":/images/china"),QString(),this);
  languagesActions << chineseLanguageAction;
  languagesSignalMapper->setMapping(chineseLanguageAction,
                                    QLocale(QLocale::Chinese,QLocale::China).name());
  connect(chineseLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  mandarinLanguageAction = new QAction(QIcon(":/images/china"),QString(),this);
  languagesActions << mandarinLanguageAction;
  languagesSignalMapper->setMapping(mandarinLanguageAction,
                                    QLocale(QLocale::Vietnamese,QLocale::China).name());
  connect(mandarinLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  catalanLanguageAction = new QAction(QIcon(":/images/catalonia"),QString(),this);
  languagesActions << catalanLanguageAction;
  languagesSignalMapper->setMapping(catalanLanguageAction,
                                    QLocale(QLocale::Catalan,QLocale::Andorra).name());
  connect(catalanLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  greekLanguageAction = new QAction(QIcon(":/images/greece"),QString(),this);
  languagesActions << greekLanguageAction;
  languagesSignalMapper->setMapping(greekLanguageAction,
                                    QLocale(QLocale::Greek,QLocale::Greece).name());
  connect(greekLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  koreanLanguageAction = new QAction(QIcon(":/images/korea"),QString(),this);
  languagesActions << koreanLanguageAction;
  languagesSignalMapper->setMapping(koreanLanguageAction,
                                    QLocale(QLocale::Korean,QLocale::RepublicOfKorea).name());
  connect(koreanLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  hungarianLanguageAction = new QAction(QIcon(":/images/hungary"),QString(),this);
  languagesActions << hungarianLanguageAction;
  languagesSignalMapper->setMapping(hungarianLanguageAction,
                                    QLocale(QLocale::Hungarian,QLocale::Hungary).name());
  connect(hungarianLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  danishLanguageAction = new QAction(QIcon(":/images/denmark"),QString(),this);
  languagesActions << danishLanguageAction;
  languagesSignalMapper->setMapping(danishLanguageAction,
                                    QLocale(QLocale::Danish,QLocale::Denmark).name());
  connect(danishLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  swedishLanguageAction = new QAction(QIcon(":/images/sweden"),QString(),this);
  languagesActions << swedishLanguageAction;
  languagesSignalMapper->setMapping(swedishLanguageAction,
                                    QLocale(QLocale::Swedish,QLocale::Sweden).name());
  connect(swedishLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  turkishLanguageAction = new QAction(QIcon(":/images/turkey"),QString(),this);
  languagesActions << turkishLanguageAction;
  languagesSignalMapper->setMapping(turkishLanguageAction,
                                    QLocale(QLocale::Turkish,QLocale::Turkey).name());
  connect(turkishLanguageAction,SIGNAL(triggered()),languagesSignalMapper,SLOT(map()));

  hiddenWidgetForLanguagesActions->addActions(languagesActions);
}

void Application::retranslateMenu()
{
  if (!playAction)
    return;

  qDebug() << "Application::retranslateMenu()";

  cancelAction->setText(tr("Cancel"));
  if (!playAction->data().toBool())
    playAction->setText(tr("Play"));
  else
    playAction->setText(tr("Pause"));
  nextPhotoAction->setText(tr("Next photo"));
  savePhotoAction->setText(tr("Save photo..."));
  getExifInfoAction->setText(tr("Get EXIF info..."));
  configureAction->setText(tr("Settings..."));
  viewSourceAction->setText(tr("Explore source"));
  aboutAction->setText(tr("About..."));
  historyAction->setText(tr("History..."));
  aboutQtAction->setText(tr("About Qt..."));
  quitAction->setText(tr("Quit"));
  languagesMenu->setTitle(tr("Languages"));

  englishLanguageAction->setText(tr("English"));
  italianLanguageAction->setText(tr("Italian"));
  spanishLanguageAction->setText(tr("Spanish"));
  germanLanguageAction->setText(tr("German"));
  frenchLanguageAction->setText(tr("French"));
  russianLanguageAction->setText(tr("Russian"));
  brazilianPortugueseLanguageAction->setText(tr("Portuguese (Brazil)"));
  czechLanguageAction->setText(tr("Czech"));
  polishLanguageAction->setText(tr("Polish"));
  chineseLanguageAction->setText(tr("Chinese (Simplified)"));
  mandarinLanguageAction->setText(tr("Chinese (Traditional)"));
  catalanLanguageAction->setText(tr("Catalan"));
  greekLanguageAction->setText(tr("Greek"));
  koreanLanguageAction->setText(tr("Korean"));
  hungarianLanguageAction->setText(tr("Hungarian"));
  danishLanguageAction->setText(tr("Danish"));
  swedishLanguageAction->setText(tr("Swedish"));
  turkishLanguageAction->setText(tr("Turkish"));

  languagesMenu->clear();
  qSort(languagesActions.begin(),languagesActions.end(),actionCaseSensitiveLessThan);
  languagesMenu->addActions(languagesActions);
}

bool Application::actionCaseSensitiveLessThan(QAction *a1, QAction *a2)
{
  return QString::localeAwareCompare(a1->text(),a2->text()) < 0;
}

void Application::changeLanguage(const QString &newLocaleName)
{
  QSettings settings;

  qDebug() << "Application::changeLanguage(" << newLocaleName.toAscii().constData() << ")";

  if (translators.contains(currentLocale.name()))
    qApp->removeTranslator(translators.value(currentLocale.name()));

  if (translators.contains(newLocaleName))
    qApp->installTranslator(translators.value(newLocaleName));

  retranslateMenu();

  currentLocale = QLocale(newLocaleName);

  settings.beginGroup(MAIN_SECTION);
  settings.setValue(LOCALE,newLocaleName);
  settings.endGroup();
}

void Application::setupMenu()
{
  qDebug() << "Application::setupMenu()";

  cancelContextMenu = new QMenu;
  cancelContextMenu->addAction(cancelAction);

  languagesMenu = new QMenu;
  languagesMenu->setIcon(QIcon(":/images/world"));
  languagesMenu->addAction(englishLanguageAction);
  languagesMenu->addAction(italianLanguageAction);
  languagesMenu->addAction(spanishLanguageAction);
  languagesMenu->addAction(germanLanguageAction);
  languagesMenu->addAction(frenchLanguageAction);
  languagesMenu->addAction(russianLanguageAction);
  languagesMenu->addAction(brazilianPortugueseLanguageAction);
  languagesMenu->addAction(czechLanguageAction);
  languagesMenu->addAction(polishLanguageAction);
  languagesMenu->addAction(chineseLanguageAction);
  languagesMenu->addAction(mandarinLanguageAction);
  languagesMenu->addAction(catalanLanguageAction);
  languagesMenu->addAction(greekLanguageAction);
  languagesMenu->addAction(koreanLanguageAction);
  languagesMenu->addAction(hungarianLanguageAction);
  languagesMenu->addAction(danishLanguageAction);
  languagesMenu->addAction(swedishLanguageAction);
  languagesMenu->addAction(turkishLanguageAction);

  contextMenu = new QMenu;
  contextMenu->addAction(playAction);
  contextMenu->addAction(nextPhotoAction);
  contextMenu->addAction(savePhotoAction);
  contextMenu->addAction(getExifInfoAction);
  contextMenu->addAction(viewSourceAction);
  contextMenu->addAction(configureAction);
  contextMenu->addSeparator();
  contextMenu->addMenu(languagesMenu);
  contextMenu->addSeparator();
  contextMenu->addAction(historyAction);
  contextMenu->addSeparator();
  contextMenu->addAction(aboutAction);
  contextMenu->addAction(aboutQtAction);
  contextMenu->addAction(quitAction);

  trayIcon->setContextMenu(contextMenu);
}

void Application::setupTrayIcon()
{
  qDebug() << "Application::setupTrayIcon()";

  trayIcon = new QSystemTrayIcon(this);
  trayIcon->setIcon(QIcon(":/images/off"));

  trayIcon->setToolTip(QString(APPLICATION_NAME) + " " + QString(APPLICATION_VERSION));
  connect(this,SIGNAL(aboutToQuit()),trayIcon,SLOT(hide()));
  connect(trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this,SLOT(execTrayAction(QSystemTrayIcon::ActivationReason)));

  trayIcon->show();
}

#ifdef Q_WS_X11
void Application::autoDetectWM()
{
  bool ok;
  QFileInfoList files = QDir("/proc").entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot);
  QListIterator<QFileInfo> file(files);
  QString debugMsg = "Application::autoDetectWM() ";

  while (file.hasNext() && (windowManager == wmUnknown))
  {
    file.peekNext().fileName().toInt(&ok);

    if (ok && QFileInfo(file.peekNext().absoluteFilePath() + "/exe").symLinkTarget().contains("xfdesktop"))
    {
      windowManager = wmXfce;
      debugMsg += "detected XFCE";
    }

    file.next();
  }

  file.toFront();
  while (file.hasNext() && (windowManager == wmUnknown))
  {
    file.peekNext().fileName().toInt(&ok);

    if (ok && QFileInfo(file.peekNext().absoluteFilePath() + "/exe").symLinkTarget().contains("gnome-shell"))
    {
      windowManager = wmGnomeShell;
      debugMsg += "detected GnomeShell";
    }

    file.next();
  }

  file.toFront();
  while (file.hasNext() && (windowManager == wmUnknown))
  {
    file.peekNext().fileName().toInt(&ok);

    if (ok)
    {
      QString fName = QFileInfo(QFileInfo(file.peekNext().absoluteFilePath() + "/exe").symLinkTarget()).fileName();

      if (fName.startsWith("kdeinit") && !fName.startsWith("kdeinit4"))
      {
        windowManager = wmKDE3;
        debugMsg += "detected KDE3";
      }
      else if (fName.startsWith("kdeinit4"))
      {
        windowManager = wmKDE4;
        debugMsg += "detected KDE4";
      }
      else if (fName.contains("gconfd-2"))
      {
        windowManager = wmGnome;
        debugMsg += "detected Gnome";
      }
      else if (fName.contains("xfdesktop"))
      {
        windowManager = wmXfce;
        debugMsg += "detected XFCE";
      }
      else if (fName.contains("fluxbox"))
      {
        windowManager = wmFluxbox;
        debugMsg += "detected FluxBox";
      }
      else if (fName.contains("fvwm"))
      {
        windowManager = wmFVWM;
        debugMsg += "detected FVWM";
      }
      else if (fName.contains("blackbox"))
      {
        windowManager = wmBlackbox;
        debugMsg += "detected BlackBox";
      }
      else if (fName.contains("wmaker"))
      {
        windowManager = wmWindowMaker;
        debugMsg += "detected WindowMaker";
      }
    }

    file.next();
  }

  if (windowManager == wmUnknown)
    debugMsg += "no WM detected";

  qDebug() << debugMsg;
}
#endif

void Application::switchToNextState(Events event)
{
  if (dontSwitchState)
    return;

  qDebug() << "Application::switchToNextState(" << eventsStrs[event] << ")" << statesStrs[currentState];

  switch (currentState)
  {
    case sIdle:
      switch (event)
      {
        case ePlayTimeout:
          sizeFailures = 0;
          currentPhotoInfo = PhotoInfo();
          currentFile = QFileInfo();
          currentState = sSelectNextEngineFirst;
          switchToNextState(eOk);
          break;

        default:
          break;
      }
      break;


    case sSelectNextEngineFirst:
      switch (event)
      {
        case eOk:
          if (!engines.size())
          {
            currentState = sIdle;
            timer->start();
          }
          else
          {
            trayIcon->setIcon(QIcon(":/images/active"));
            trayIcon->setContextMenu(cancelContextMenu);
            markEngineIndex = currentEngineIndex = (currentEngineIndex + 1) % engines.size();
            if (!engines.at(currentEngineIndex)->isActive())
            {
              currentEngineIndex = (currentEngineIndex + 1) % engines.size();
              while (!engines.at(currentEngineIndex)->isActive() && (markEngineIndex != currentEngineIndex))
                currentEngineIndex = (currentEngineIndex + 1) % engines.size();

              if (currentEngineIndex == markEngineIndex)
              {
                trayIcon->setIcon(QIcon((playWasActive)? ":/images/idle" : ":/images/off"));
                trayIcon->setContextMenu(contextMenu);
                currentState = sIdle;
                timer->start();
              }
              else
              {
                currentState = sInitEngine;
                selectEngine(currentEngineIndex);
                emit init();
              }
            }
            else
            {
              currentState = sInitEngine;
              selectEngine(currentEngineIndex);
              emit init();
            }
          }
          break;

        default:
          break;
      }
      break;

    case sInitEngine:
      switch (event)
      {
        case eOk:
          currentState = sSearchRequest;
          emit search(settings.value(MAIN_SECTION).value(RANDOM_SEARCH).toBool());
          break;

        case eCancel:
          currentState = sCancel;
          emit cancel();
          break;

        default:
          break;
      }
      break;

    case sSearchRequest:
      switch (event)
      {
        case eOk:
          currentState = sSizeRequest;
          emit sizeLookup();
          break;

        case eSearchNoData:
          currentState = sSelectNextEngineNext;
          switchToNextState(eOk);
          break;

        case eCancel:
          currentState = sCancel;
          emit cancel();
          break;

        default:
          break;
      }
      break;

    case sSelectNextEngineNext:
      switch (event)
      {
        case eOk:
          currentEngineIndex = (currentEngineIndex + 1) % engines.size();
          while (!engines.at(currentEngineIndex)->isActive() && (markEngineIndex != currentEngineIndex))
            currentEngineIndex = (currentEngineIndex + 1) % engines.size();
          if (currentEngineIndex == markEngineIndex)
          {
            trayIcon->setIcon(QIcon((playWasActive)? ":/images/idle" : ":/images/off"));
            trayIcon->setContextMenu(contextMenu);
            currentState = sIdle;
            timer->start();
          }
          else
          {
            currentState = sInitEngine;
            selectEngine(currentEngineIndex);
            emit init();
          }
          break;

        default:
          break;
      }
      break;

    case sSizeRequest:
      switch (event)
      {
        case eOk:
          currentState = sDownload;
          emit download();
          break;

        case eWrongSize:
          if (++sizeFailures == MAX_SIZE_FAILURES)
          {
            saveEnginesState();
            currentState = sSearchRequest;
            switchToNextState(eSearchNoData);
          }
          else
          {
            currentState = sInitEngine;
            emit init();
          }
          break;

        case eCancel:
          currentState = sCancel;
          emit cancel();
          break;

        default:
          break;
      }
      break;

    case sDownload:
      switch (event)
      {
        case eOk:
          currentState = sInfoRequest;
          emit infoCollect();
          break;

        case eCancel:
          currentState = sCancel;
          emit cancel();
          break;

        case eDownloadFailed:
          saveEnginesState();
          trayIcon->setIcon(QIcon((playWasActive)? ":/images/idle" : ":/images/off"));
          trayIcon->setContextMenu(contextMenu);
          currentState = sIdle;
          timer->start();
          break;

        case eDownloadOkButWrongSize:
          if (++sizeFailures == MAX_SIZE_FAILURES)
          {
            saveEnginesState();
            currentState = sSearchRequest;
            switchToNextState(eSearchNoData);
          }
          else
          {
            currentState = sInitEngine;
            emit init();
          }
          break;

        default:
          break;
      }
      break;

    case sInfoRequest:
      switch (event)
      {
        case eOk:
          saveEnginesState();
          tryCompletePhotoInfo();
          updateDB();
          showPhotoOnScreen();
          savePhotoAction->setEnabled(true);
          #ifdef EXIF
            getExifInfoAction->setEnabled(true);
          #endif
          trayIcon->setIcon(QIcon((playWasActive)? ":/images/idle" : ":/images/off"));
          trayIcon->setContextMenu(contextMenu);
          currentState = sIdle;
          timer->start();
          break;

        case eCancel:
          saveEnginesState();
          tryCompletePhotoInfo();
          updateDB();
          showPhotoOnScreen();
          savePhotoAction->setEnabled(true);
          #ifdef EXIF
            getExifInfoAction->setEnabled(true);
          #endif
          currentState = sCancel;
          emit cancel();
          break;

        default:
          break;
      }
      break;

    case sCancel:
      switch (event)
      {
        case eOk:
          trayIcon->setIcon(QIcon((playWasActive)? ":/images/idle" : ":/images/off"));
          trayIcon->setContextMenu(contextMenu);
          currentState = sIdle;
          timer->start();
          break;

        default:
          break;
      }
      break;
  }
}

void Application::execTrayAction(QSystemTrayIcon::ActivationReason reason)
{
  switch (reason)
  {
    case QSystemTrayIcon::DoubleClick:
      if ((currentState == sIdle) && playAction->isEnabled())
        nextPhotoAction->trigger();

    default:
      break;
  }
}

void Application::start()
{
  switchToNextState(ePlayTimeout);
}

void Application::initCompleted()
{
  switchToNextState(eOk);
}

void Application::processSearchResult(bool hasData)
{
  qDebug() << "Application::processSearchResult(" << hasData << ")";

  switchToNextState((hasData)? eOk : eSearchNoData);
}

bool Application::isSizeValid(const QSize &size) const
{
  bool ok;

  if (size.isValid() && ((size.width() * size.height()) < MAX_PIXEL_NUMBER))
  {
    switch (settings.value(MAIN_SECTION).value(SIZE_CONSTRAINT).toInt())
    {
      case 0:
        ok = true;
        break;

      case 1:
        ok = (size.width() * size.height()) >=
             (0.5 * static_cast<double> (QApplication::desktop()->screenGeometry().width()) *
                    static_cast<double> (QApplication::desktop()->screenGeometry().height()));
        break;

      case 2:
        ok = (size.width() * size.height()) >=
             (0.75 * static_cast<double> (QApplication::desktop()->screenGeometry().width()) *
                     static_cast<double> (QApplication::desktop()->screenGeometry().height()));
        break;

      case 3:
        ok = (size.width() * size.height()) >=
             (QApplication::desktop()->screenGeometry().width() *
              QApplication::desktop()->screenGeometry().height());
        break;

      default:
        ok = false;
        break;
    }

    if (settings.value(MAIN_SECTION).value(ONLY_LANDSCAPES).toBool())
      ok &= size.width() >= ((4 * size.height()) / 3);

    return ok;
  }
  else
    return false;
}

void Application::processPhotoSize(const QSize &size)
{
  qDebug() << "Application::processPhotoSize(" << size.width() << "x" << size.height() << ")";

  switchToNextState((isSizeValid(size))? eOk : eWrongSize);
}

void Application::processDownloadedPhoto(bool ok, const QFileInfo &localFile)
{
  QList<QFileInfo> files;

  qDebug() << "Application::processDownloadedPhoto(" << ok << ","
           << localFile.absoluteFilePath().toAscii().constData() << ")";

  if (ok && QImageReader(localFile.absoluteFilePath()).canRead())
  {
    QSize size = QImageReader(localFile.absoluteFilePath()).size();

    if (isSizeValid(size))
    {
      files = QDir(_tempStorageDir).entryInfoList(QDir::Files);
      QListIterator<QFileInfo> file(files);

      while (file.hasNext())
      {
        if ((file.peekNext() != localFile) && file.peekNext().fileName().compare("wallykde4.img",Qt::CaseInsensitive))
        {
          QFile::remove(file.peekNext().absoluteFilePath());
          qDebug() << "\tdelete " << file.peekNext().absoluteFilePath().toAscii().constData();
        }

        file.next();
      }

      currentFile = localFile;
      currentPhotoSize = size;

      switchToNextState(eOk);
    }
    else
    {
      qDebug() << "\tsize: " << size.width() << "x" << size.height();

      switchToNextState(eDownloadOkButWrongSize);
    }
  }
  else
    switchToNextState(eDownloadFailed);
}

void Application::processCollectedInfo(const PhotoInfo &info)
{
  qDebug() << "Application::processCollectedInfo(" << info.owner.toAscii().constData() << ","
           << info.title.toAscii().constData() << ")";

  currentPhotoInfo = info;
  viewSourceAction->setEnabled(!currentPhotoInfo.sourceUrl.isEmpty() &&
                               currentPhotoInfo.sourceUrl.isValid());
  switchToNextState(eOk);
}

void Application::tryCompletePhotoInfo()
{
#ifdef EXIF
  Exif::Tags tags(currentFile);

  if (currentPhotoInfo.owner.isEmpty())
    currentPhotoInfo.owner = tags.owner();

  if (currentPhotoInfo.title.isEmpty())
    currentPhotoInfo.title = tags.title();

  if (currentPhotoInfo.description.isEmpty())
    currentPhotoInfo.description = tags.description();
#endif

  if (currentPhotoInfo.title.isEmpty())
    currentPhotoInfo.title = currentFile.baseName();
}

void Application::switchToIdle()
{
  qDebug() << "Application::switchToIdle()";

  switchToNextState(eOk);
}

void Application::cleanHistory(int now, int timeBack, int timeBackFactor)
{
  QSqlQuery query(db);
  QDirIterator thumbIterator(_tempStorageDir + "/thumbs",QStringList() << "*.png",QDir::Files);
  QDateTime dateTime = QDateTime::fromTime_t(now);

  switch (timeBackFactor)
  {
    case 0:
      dateTime = dateTime.addDays(-timeBack);
      break;

    case 1:
      dateTime = dateTime.addMonths(-timeBack);
      break;
  }

  if (timeBack)
  {
    query.prepare(QString("delete from photoHistory where timestamp <= :timestamp"));
    query.bindValue(":timestamp",dateTime.toTime_t());
    query.exec();
  }
  else
    query.exec("delete from photoHistory");

  query.exec("vacuum");

  while (thumbIterator.hasNext())
  {
    thumbIterator.next();

    if (!timeBack || (thumbIterator.fileInfo().baseName().toUInt() <= dateTime.toTime_t()))
      QFile::remove(thumbIterator.fileInfo().absoluteFilePath());
  }
}

void Application::updateDBVersion()
{
  QSqlQuery query(db);

  if (query.exec("pragma user_version"))
  {
    query.next();

    int user_version = query.value(0).toInt();

    while (user_version < DATABASE_CURRENT_VERSION)
      switch (user_version)
      {
        case 0:
          qDebug() << "Application::updateDBVersion() " << user_version << "==>" << (user_version + 1);
          if (!query.exec("alter table photoHistory add column tags text") ||
              !query.exec("alter table photoHistory add column width integer") ||
              !query.exec("alter table photoHistory add column height integer") ||
              !query.exec("alter table photoHistory add column exif blob default null") ||
              !query.exec("update photoHistory set tags = 'unknown'"))
          {
            qDebug() << "Application::updateDB() error:" << query.lastError().text();
            user_version = DATABASE_CURRENT_VERSION;
            db.close();
          }
          else
            ++user_version;
          break;
      }

    query.exec(QString("pragma user_version = %1").arg(DATABASE_CURRENT_VERSION));
  }
}

void Application::initializeDB()
{
  qDebug() << "Application::initializeDB()";

  db = QSqlDatabase::addDatabase("QSQLITE");
  QDir(_tempStorageDir).mkdir("thumbs");
  QDir(_tempStorageDir).mkdir("images");
  db.setDatabaseName(_tempStorageDir + "/thumbs/wally.db");
  if (db.open())
  {
    QSqlQuery query(db);

    query.exec("PRAGMA journal_mode = OFF");

    if (query.exec("select * from sqlite_master where name = 'photoHistory'"))
    {
      if (!query.next() &&
          (!query.exec(QString("create table photoHistory (") +
                      "  timestamp integer primary key, " +
                      "  url text, " +
                      "  engine text, " +
                      "  tags text default 'unknown', " +
                      "  title text, " +
                      "  owner text, " +
                      "  description text, " +
                      "  location text, " +
                      "  sourceUrl text, " +
                      "  size integer, " +
                      "  width integer, " +
                      "  height integer, " +
                      "  exif blob default null)") ||
           !query.exec(QString("pragma user_version = %1").arg(DATABASE_CURRENT_VERSION))))
      {
        db.close();

        qDebug() << "Application::Application() cannot create photoHistory table";
      }
      else
        updateDBVersion();
    }
  }
  else
    qDebug() << "Application::Application() db connection failed";
}

void Application::updateDB()
{
  Exif::Tags tags(currentFile);
  QByteArray cTags;
  int now = QDateTime::currentDateTime().toTime_t();

  qDebug() << "Application::updateDB()";

  cleanHistory(now,settings.value(MAIN_SECTION).value(HISTORY_TIME_LIMIT).toInt(),
               settings.value(MAIN_SECTION).value(HISTORY_TIME_LIMIT_FACTOR).toInt());

  if (currentPhotoInfo.searchString.isEmpty())
    return;

  if (db.isOpen() && !engines.at(currentEngineIndex)->photoUrl().isEmpty() &&
      settings.value(MAIN_SECTION).value(HISTORY_TIME_LIMIT).toInt())
  {
    QSqlQuery query(db);

    if (tags.size())
      query.prepare(QString("insert into photoHistory (timestamp, url, engine, tags, title, owner, ") +
                    "description, location, sourceUrl, size, width, height, exif) values (" +
                    ":timestamp, :url, :engine, :tags, :title, :owner, " +
                    ":description, :location, :sourceUrl, :size, :width, :height, :exif)");
    else
      query.prepare(QString("insert into photoHistory (timestamp, url, engine, tags, title, owner, ") +
                    "description, location, sourceUrl, size, width, height, exif) values (" +
                    ":timestamp, :url, :engine, :tags, :title, :owner, " +
                    ":description, :location, :sourceUrl, :size, :width, :height, null)");

    query.bindValue(":timestamp",now);
    query.bindValue(":url",engines.at(currentEngineIndex)->photoUrl());
    query.bindValue(":engine",engines.at(currentEngineIndex)->name());
    query.bindValue(":tags",currentPhotoInfo.searchString.toLower());
    query.bindValue(":title",currentPhotoInfo.title);
    query.bindValue(":owner",currentPhotoInfo.owner);
    query.bindValue(":description",currentPhotoInfo.description);
    query.bindValue(":location",currentPhotoInfo.location);
    query.bindValue(":sourceUrl",currentPhotoInfo.sourceUrl.toString());
    query.bindValue(":size",currentFile.size());
    query.bindValue(":width",currentPhotoSize.width());
    query.bindValue(":height",currentPhotoSize.height());
    if (tags.size())
    {
      cTags = qCompress(tags.data());
      query.bindValue(":exif",cTags,QSql::Binary | QSql::In);
    }

    if (query.exec())
    {
      QImage thumb(currentFile.absoluteFilePath());

      if (thumb.width() > thumb.height())
        thumb = thumb.scaledToWidth(100,Qt::SmoothTransformation);
      else
        thumb = thumb.scaledToHeight(100,Qt::SmoothTransformation);

      if (settings.value(MAIN_SECTION).value(EXIF_ROTATE_IMAGES).toBool())
        thumb = Exif::Tags(currentFile).normalize(thumb);

      thumb.save(_tempStorageDir + "/thumbs/" + QString::number(now) + ".png","png");
    }
    else
      qDebug() << "Application::updateDB() insert error:" << query.lastError().text();
  }
}

void Application::clearHistory()
{
  QSqlQuery query(db);
  QDirIterator dirIterator(_tempStorageDir + "/thumbs",
                           QStringList() << "*.png",
                           QDir::Files);

  qDebug() << "Application::clearHistory()";

  if (!query.exec("delete from photoHistory") ||
      !query.exec("vacuum"))
    qDebug() << "Application::clearHistory() delete error:" << query.lastError().text();

  while (dirIterator.hasNext())
  {
    dirIterator.next();
    QFile::remove(dirIterator.filePath());
  }
}

void Application::showPhotoOnScreen()
{
  QStringList args;
  QString toolTip;
  QStringList filters;
  QList<QFileInfo> files;
  QFileInfo newFile;
  QFile textFile;
  QTextStream textStream;
  bool lastWasTitle = false;

  qDebug() << "Application::showPhotoOnScreen()";

  try
  {
  #ifdef Q_WS_X11
    if (QFileInfo(QDir(_tempStorageDir),"scripts/wally.sh").exists())
    {
      newFile = adaptPhoto(currentFile.absoluteFilePath(),"PNG",currentPhotoInfo);
      QProcess::execute(QFileInfo(QDir(_tempStorageDir),"scripts/wally.sh").absoluteFilePath(),
                        QStringList() << newFile.absoluteFilePath());
    }
    else
      switch (windowManager)
      {
        case wmKDE4:
          {
            newFile = adaptPhoto(currentFile.absoluteFilePath(),"PNG",currentPhotoInfo);

            QDBusMessage message = QDBusMessage::createMethodCall("com.BeCrux.WallyPlugin",
                                                                  "/WallyPlugin",
                                                                  QString(),
                                                                  "setWallpaper");
            message.setArguments(QList< QVariant >() << newFile.absoluteFilePath());
            QDBusConnection::sessionBus().send(message);
          }
          break;

        case wmKDE3:
          newFile = adaptPhoto(currentFile.absoluteFilePath(),"PNG",currentPhotoInfo);
          args << "kdesktop" << "KBackgroundIface" << "setWallpaper"
               << newFile.absoluteFilePath() << "1";
          QProcess::execute("dcop",args);
          break;

        case wmGnomeShell:
        case wmGnome:
          newFile = adaptPhoto(currentFile.absoluteFilePath(),"PNG",currentPhotoInfo);
          args << "--type" << "bool" << "--set" <<
                  "/desktop/gnome/background/draw_background" << "true";
          QProcess::execute("gconftool-2",args);
          args.clear();
          args << "--type" << "string" << "--set" <<
                  "/desktop/gnome/background/picture_options" << "centered";
          QProcess::execute("gconftool-2",args);
          args.clear();
          args << "--type" << "string" << "--set" <<
                  "/desktop/gnome/background/picture_filename" << newFile.absoluteFilePath();
          QProcess::execute("gconftool-2",args);

          args.clear();
          newFile = adaptPhoto(currentFile.absoluteFilePath(),"PNG",currentPhotoInfo);
          args << "set" << "org.gnome.desktop.background" << "draw-background" << "true";
          QProcess::execute("gsettings",args);
          args.clear();
          args << "set" << "org.gnome.desktop.background" << "picture-options" << "centered";
          QProcess::execute("gsettings",args);
          args.clear();
          args << "set" << "org.gnome.desktop.background" << "picture-uri" << QUrl::fromLocalFile(newFile.absoluteFilePath()).toString();
          QProcess::execute("gsettings",args);
          break;

        case wmXfce:
          newFile = adaptPhoto(currentFile.absoluteFilePath(),"PNG",currentPhotoInfo);

          {
            QProcess p;

            p.start("xfconf-query",QStringList() << "-c" << "xfce4-desktop" << "-p" << "/backdrop" << "-lv",QIODevice::ReadOnly);
            p.waitForFinished();

            QStringList monitors;
            QStringList workspaces;

            if (p.exitStatus() == QProcess::NormalExit)
            {
              QTextStream r(&p);
              QString line;

              while (!(line = r.readLine()).isNull())
                if (line.contains("image-path"))
                {
                  QFileInfo fi(line.section(" ",0,0));
                  monitors << fi.path();
                }
                else if (line.contains("last-image"))
                {
                  QFileInfo fi(line.section(" ",0,0));
                  workspaces << fi.path();
                }
            }

            if (!monitors.isEmpty() || !workspaces.isEmpty())
            {
              QStringListIterator mIter(monitors);
              while (mIter.hasNext())
              {
                QString ms(mIter.next());
                QProcess::execute("xfconf-query",QStringList() << "-c"
                                                               << "xfce4-desktop"
                                                               << "-p"
                                                               << QString("%1/image-show").arg(ms)
                                                               << "-s"
                                                               << "true");

                QProcess::execute("xfconf-query",QStringList() << "-c"
                                                               << "xfce4-desktop"
                                                               << "-p"
                                                               << QString("%1/image-style").arg(ms)
                                                               << "-s"
                                                               << "1");

                QProcess::execute("xfconf-query",QStringList() << "-c"
                                                               << "xfce4-desktop"
                                                               << "-p"
                                                               << QString("%1/image-path").arg(ms)
                                                               << "-s"
                                                               << newFile.absoluteFilePath());
              }

              QStringListIterator wIter(workspaces);
              while (wIter.hasNext())
              {
                QString ws(wIter.next());
                QProcess::execute("xfconf-query",QStringList() << "-c"
                                                               << "xfce4-desktop"
                                                               << "-p"
                                                               << QString("%1/color-style").arg(ws)
                                                               << "-s"
                                                               << "3");

                QProcess::execute("xfconf-query",QStringList() << "-c"
                                                               << "xfce4-desktop"
                                                               << "-p"
                                                               << QString("%1/image-style").arg(ws)
                                                               << "-s"
                                                               << "1");

                QProcess::execute("xfconf-query",QStringList() << "-c"
                                                               << "xfce4-desktop"
                                                               << "-p"
                                                               << QString("%1/last-image").arg(ws)
                                                               << "-s"
                                                               << newFile.absoluteFilePath());
              }
            }
            else
            {
              textFile.setFileName(QDir::homePath() + "/.config/xfce4/desktop/backdrops.list");
              textFile.open(QIODevice::WriteOnly);
              textStream.setDevice(&textFile);

              textStream << "# xfce backdrop list" << endl;
              textStream << newFile.absoluteFilePath() << endl;
              textFile.close();

              args << "--reload";
              QProcess::execute("xfdesktop",args);
            }
          }
          break;

        case wmFluxbox:
          newFile = adaptPhoto(currentFile.absoluteFilePath(),"PNG",currentPhotoInfo);
          args << "-c" << newFile.absoluteFilePath();
          QProcess::execute("fbsetbg",args);
          break;

        case wmFVWM:
          newFile = adaptPhoto(currentFile.absoluteFilePath(),"XPM",currentPhotoInfo);
          args << newFile.absoluteFilePath();
          QProcess::execute("fvwm-root",args);
          break;

        case wmBlackbox:
          newFile = adaptPhoto(currentFile.absoluteFilePath(),"PNG",currentPhotoInfo);
          args << "-center" << newFile.absoluteFilePath();
          QProcess::execute("bsetbg",args);
          break;

        case wmWindowMaker:
          newFile = adaptPhoto(currentFile.absoluteFilePath(),"PNG",currentPhotoInfo);
          args << "-e" << newFile.absoluteFilePath();
          QProcess::execute("wmsetbg",args);
          break;

        default:
          break;
      }
  #endif

  #ifdef Q_WS_WIN
    SHELLFLAGSTATE shfs;

    SHGetSettings(&shfs,SSF_DESKTOPHTML);
    if (shfs.fDesktopHTML)
    {
      if (!adWarningFired)
      {
        adWarningFired = true;
        trayIcon->showMessage(tr("Wally Error"),tr("Active Desktop must be disabled"),QSystemTrayIcon::Critical);
      }

      return;
    }

    newFile = adaptPhoto(currentFile.absoluteFilePath(),"BMP",currentPhotoInfo);

    QSettings regSettings("HKEY_CURRENT_USER\\Control Panel\\Desktop",QSettings::NativeFormat);
    regSettings.setValue("Wallpaper",QDir::toNativeSeparators(newFile.absoluteFilePath()));
    regSettings.setValue("WallpaperStyle","0");
    regSettings.setValue("TileWallpaper","0");

    #ifdef UNICODE
      WCHAR path[1024];

      memset(path,0,1024 * sizeof(WCHAR));
      QDir::toNativeSeparators(newFile.absoluteFilePath()).toWCharArray(path);

      SystemParametersInfoW(SPI_SETDESKWALLPAPER,0,reinterpret_cast<void *> (path),SPIF_SENDWININICHANGE);
    #else
      SystemParametersInfoA(SPI_SETDESKWALLPAPER,0,
                            reinterpret_cast<void *>
                              (QDir::toNativeSeparators(newFile.absoluteFilePath()).toAscii().data()),
                            SPIF_SENDWININICHANGE);
    #endif
  #endif

  #ifdef Q_WS_PM
    newFile = adaptPhoto(currentFile.absoluteFilePath(),"BMP",currentPhotoInfo);

    HOBJECT hobj;
    const char *objName = "<WP_DESKTOP>";

      if (hobj = WinQueryObject(reinterpret_cast< const unsigned char * > (objName)))
        WinSetObjectData(hobj,reinterpret_cast< const unsigned char * > ((QString("BACKGROUND=%1,S,1,I;").arg(QDir::toNativeSeparators(newFile.absoluteFilePath()))).toAscii().constData()));

  #endif

  #ifdef Q_WS_MAC
    newFile = adaptPhoto(currentFile.absoluteFilePath(),"PNG",currentPhotoInfo);
    QFile scriptFile(_tempStorageDir + "/wally.scpt");
    QTextStream scriptText(&scriptFile);

    scriptFile.open(QIODevice::WriteOnly);

    if (QSysInfo::MacintoshVersion == QSysInfo::MV_10_6)
    {
      scriptText << "tell application \"System Events\"" << endl;
      scriptText << "set picture of every desktop to POSIX file \"" << newFile.absoluteFilePath() << "\"" << endl;
      scriptText << "end tell" << endl;
    }
    else
    {
      scriptText << "tell application \"Finder\"" << endl;
      scriptText << "set desktop picture to POSIX file \"" << newFile.absoluteFilePath() << "\"" << endl;
      scriptText << "end tell" << endl;
    }
    scriptFile.close();

    args << QDir::toNativeSeparators(_tempStorageDir + "/wally.scpt");
    QProcess::execute("osascript",args);
  #endif

    if (settings.value(MAIN_SECTION).value(VIEW_INFO_IN_TOOLTIP).toBool())
    {
      if (!currentPhotoInfo.owner.isEmpty() ||
          !currentPhotoInfo.title.isEmpty() ||
          !currentPhotoInfo.description.isEmpty() ||
          !currentPhotoInfo.location.isEmpty())
      {
        if (!currentPhotoInfo.title.isEmpty())
          toolTip += QString("<b>\"") + currentPhotoInfo.title + "\"</b>";
        lastWasTitle = true;
        if (!currentPhotoInfo.description.isEmpty())
        {
          if (!toolTip.isEmpty())
            toolTip += QString("<br><br>");
          if (lastWasTitle)
            toolTip += QString("<br>");
          toolTip += QString("<i>") + currentPhotoInfo.description + "</i>";
          lastWasTitle = false;
        }
        if (!currentPhotoInfo.owner.isEmpty())
        {
          if (!toolTip.isEmpty())
            toolTip += QString("<br>");
          if (lastWasTitle)
            toolTip += QString("<br>");
          toolTip += tr("by:") + " " + currentPhotoInfo.owner;
          lastWasTitle = false;
        }
        if (!currentPhotoInfo.location.isEmpty())
        {
          if (!toolTip.isEmpty())
            toolTip += QString("<br>");
          if (lastWasTitle)
            toolTip += QString("<br>");
          toolTip += tr("Location:") + " " + currentPhotoInfo.location;
          lastWasTitle = false;
        }
      }
      if (!toolTip.isEmpty())
        toolTip += QString("<br>");
      if (lastWasTitle)
        toolTip += QString("<br>");
      toolTip += tr("Engine:") + " " + engines.at(currentEngineIndex)->name();

      trayIcon->setToolTip(toolTip);
    }

    if (settings.value(MAIN_SECTION).value(QUIT_AFTER_BACKGROUND_CHANGE).toBool())
      QTimer::singleShot(10000,quitAction,SLOT(trigger()));
  }
  catch (...)
  {
    qDebug() << "Application::showPhotoOnScreen() !!photo is not valid!!";
  }
}

void Application::showPhotoOnScreen(const QString &engineName)
{
  int i = 0;

  qDebug() << "Application::showPhotoOnScreen(" << engineName << ")";

  while ((i < engines.size()) && engineName.compare(engines.at(i)->name(),Qt::CaseInsensitive)) ++i;

  if (i < engines.size())
  {
    int tempEngineIndex = currentEngineIndex;
    currentEngineIndex = i;
    showPhotoOnScreen();
    currentEngineIndex = tempEngineIndex;
  }
}

bool Application::isCenterInsideDesktop(const QPoint &center, const QSize &pixmapSize, const QSize &desktopSize)
{
  return (((center.x() + pixmapSize.width() / 2) >= 0) &&
          ((center.y() + pixmapSize.height() / 2) >= 0) &&
          ((center.x() - pixmapSize.width() / 2) <= desktopSize.width()) &&
          ((center.y() - pixmapSize.width() / 2) <= desktopSize.height()));
}

bool Application::isCenterStored(const QPoint &center, const QList< QPair< QPoint,QPair<bool,bool> > > &centers)
{
  bool found = false;
  QListIterator< QPair< QPoint,QPair<bool,bool> > > c(centers);

  while (c.hasNext() && !(found = (c.next().first == center))) ;

  return found;
}

void Application::recFillCentersList(const QSize &desktopSize, const QSize &pixmapSize,
                                     QList< QPair< QPoint,QPair<bool,bool> > > &centers, const QPoint &center,
                                     bool horizontalFlip, bool verticalFlip)
{
  if (isCenterInsideDesktop(center,pixmapSize,desktopSize) &&
      !isCenterStored(center,centers))
  {
    centers.push_back(qMakePair(center,qMakePair(horizontalFlip,verticalFlip)));

    recFillCentersList(desktopSize,pixmapSize,centers,
                       QPoint(center.x(),center.y() - pixmapSize.height()),horizontalFlip,!verticalFlip);
    recFillCentersList(desktopSize,pixmapSize,centers,
                       QPoint(center.x(),center.y() + pixmapSize.height()),horizontalFlip,!verticalFlip);
    recFillCentersList(desktopSize,pixmapSize,centers,
                       QPoint(center.x() - pixmapSize.width(),center.y()),!horizontalFlip,verticalFlip);
    recFillCentersList(desktopSize,pixmapSize,centers,
                       QPoint(center.x() + pixmapSize.width(),center.y()),!horizontalFlip,verticalFlip);
  }
}

void Application::fillCentersList(const QSize &desktopSize, QSize pixmapSize,
                                  QList< QPair< QPoint,QPair<bool,bool> > > &centers)
{
  switch (static_cast<Position> (settings.value(MAIN_SECTION).value(WALLPAPER_POSITION).toInt()))
  {
    case Centered:
    case CenteredMaxpect:
    case Scaled:
    case CenteredAutoFit:
    case ScaleAndCrop:
      centers.push_back(qMakePair(QPoint(desktopSize.width() / 2,desktopSize.height() / 2),
                                  qMakePair(false,false)));
      break;

    case TiledMaxpect:
      pixmapSize.scale(desktopSize,Qt::KeepAspectRatio);

    case Tiled: /* or tiled maxpect */
    case MirroredTiled:
      recFillCentersList(desktopSize,pixmapSize,centers,
                         QPoint(pixmapSize.width() / 2,pixmapSize.height() / 2),
                         false,false);
      break;

    case CenterTiled:
      recFillCentersList(desktopSize,pixmapSize,centers,
                         QPoint(desktopSize.width() / 2,desktopSize.height() / 2),
                         false,false);
      break;

    case SymmetricalTiled:
    case SymmetricalMirroredTiled:
      recFillCentersList(desktopSize,pixmapSize,centers,
                         QPoint(desktopSize.width() / 2,desktopSize.height() / 2) -
                         QPoint(pixmapSize.width() / 2,pixmapSize.height() / 2),
                         false,false);
      break;

    default:
      break;
  }
}

void Application::drawTextWithShadowOnScene(const QString &text, QGraphicsScene *scene,
                                            Qt::AlignmentFlag alignment, int &y, bool isTitle)
{
  qreal x;
  QGraphicsTextItem *textItem;
  QFont font(qApp->font());

  font.setPointSize(10);
  font.setBold(isTitle);

  textItem = new QGraphicsTextItem;
  scene->addItem(textItem);
  textItem->setFont(font);
  textItem->setDefaultTextColor(Qt::white);
  if (isTitle)
    textItem->setHtml("\"" + text + "\"");
  else
    textItem->setHtml(text);

  switch (alignment)
  {
    case Qt::AlignRight:
    default:
      x = scene->width() - textItem->boundingRect().toRect().width();
      break;

    case Qt::AlignLeft:
      x = 2;
      break;
  }
  textItem->setZValue(1.0);
  textItem->setPos(x,y);

  textItem = new QGraphicsTextItem;
  scene->addItem(textItem);
  textItem->setFont(font);
  textItem->setDefaultTextColor(Qt::black);
  if (isTitle)
    textItem->setHtml("\"" + text + "\"");
  else
    textItem->setHtml(text);
  textItem->setPos(x+1,y+1);
  textItem->setZValue(0.5);

  y += textItem->boundingRect().toRect().height() + 1;
}

QFileInfo Application::adaptPhoto(const QFileInfo &file, const QString &format, const PhotoInfo &info)
{
  int prevHeight;
  QString newFileName;
  QList< QPair< QPoint,QPair<bool,bool> > > centers;
  QRect userRect = (settings.value(MAIN_SECTION).value(USE_FULL_DESKTOP_AREA).toBool())?
                   QApplication::desktop()->screenGeometry() :
                   QApplication::desktop()->availableGeometry();
  QGraphicsScene desktopScene(QApplication::desktop()->screenGeometry());
  QImage image;
  Position position = static_cast<Position> (settings.value(MAIN_SECTION).value(WALLPAPER_POSITION).toInt());

  qDebug() << "Application::adaptPhoto(" << file.absoluteFilePath().toAscii().constData() << ","
           << format.toAscii().constData() << ","
           << info.title.toAscii().constData() << "," << info.owner.toAscii().constData() << ")";

  qDebug() << "\tscreen: " << QApplication::desktop()->screenGeometry().left() << ","
           << QApplication::desktop()->screenGeometry().top() << ","
           << QApplication::desktop()->screenGeometry().right() << ","
           << QApplication::desktop()->screenGeometry().bottom() << ","
           << QApplication::desktop()->screenGeometry().width() << ","
           << QApplication::desktop()->screenGeometry().height();

  qDebug() << "\tavailable: " << userRect.left() << ","
           << userRect.top() << ","
           << userRect.right() << ","
           << userRect.bottom() << ","
           << userRect.width() << ","
           << userRect.height();

  if (!image.load(file.absoluteFilePath()))
    throw;

  if (settings.value(MAIN_SECTION).value(EXIF_ROTATE_IMAGES).toBool())
    image = Exif::Tags(file).normalize(image);

  QImage newImage(QApplication::desktop()->screenGeometry().width(),
                  QApplication::desktop()->screenGeometry().height(),QImage::Format_ARGB32);
  QPainter painter(&newImage);

  painter.setRenderHint(QPainter::Antialiasing,true);
  painter.setRenderHint(QPainter::TextAntialiasing,true);

  switch (position)
  {
    case Centered:
    case Tiled:
    case CenterTiled:
    default:
      break;

    case CenteredMaxpect:
    case TiledMaxpect:
      image = image.scaled(userRect.size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
      break;

    case Scaled:
      image = image.scaled(userRect.size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
      break;

    case CenteredAutoFit:
      if ((image.size().width() > userRect.width()) ||
          (image.size().height() > userRect.height()))
        image = image.scaled(userRect.size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
      break;

    case ScaleAndCrop:
      image = image.scaled(userRect.size(),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation);
      break;
  }

  if (settings.value(MAIN_SECTION).value(AUTO_COLOR).toBool())
  {
    int x, y, r = 0, g = 0, b = 0;

    for (y = 0; y < image.height(); ++y)
      for (x = 0; x < image.width(); ++x)
      {
        QColor color = image.pixel(x,y);

        r += color.red();
        g += color.green();
        b += color.blue();
      }

    desktopScene.setBackgroundBrush(QColor(r / (image.height() *  image.width()),
                                           g / (image.height() *  image.width()),
                                           b / (image.height() *  image.width())));
  }
  else
    desktopScene.setBackgroundBrush(QColor(settings.value(MAIN_SECTION).value(BORDER_COLOR).toString()));
  fillCentersList(userRect.size(),image.size(),centers);

  QListIterator< QPair< QPoint,QPair<bool,bool> > > center(centers);
  while (center.hasNext())
  {
    QPair< QPoint,QPair<bool,bool> > c = center.next();
    QPoint p(c.first.x() - image.width() / 2 + userRect.left(),
             c.first.y() - image.height() / 2 + userRect.top());
    desktopScene.addPixmap(QPixmap::fromImage(image.mirrored(((position == MirroredTiled) ||
                                                              (position == SymmetricalMirroredTiled)) && c.second.first,
                                                             ((position == MirroredTiled) ||
                                                              (position == SymmetricalMirroredTiled)) && c.second.second)))->setPos(p);
  }

  if (settings.value(MAIN_SECTION).value(VIEW_INFO_ON_PHOTO).toBool())
  {
    prevHeight = QApplication::desktop()->availableGeometry().top();
    if (!info.title.isEmpty())
      drawTextWithShadowOnScene(info.title,&desktopScene,
                                static_cast<Qt::AlignmentFlag>
                                  (settings.value(MAIN_SECTION).value(INFO_POSITION_ON_PHOTO).toInt()),
                                prevHeight,true);

    if (!info.owner.isEmpty())
      drawTextWithShadowOnScene(tr("by:") + " " + info.owner,&desktopScene,
                                static_cast<Qt::AlignmentFlag>
                                  (settings.value(MAIN_SECTION).value(INFO_POSITION_ON_PHOTO).toInt()),
                                prevHeight);

    drawTextWithShadowOnScene(tr("Engine:") + " " + engines.at(currentEngineIndex)->name(),
                              &desktopScene,
                              static_cast<Qt::AlignmentFlag>
                                (settings.value(MAIN_SECTION).value(INFO_POSITION_ON_PHOTO).toInt()),
                              prevHeight);
  }

  desktopScene.render(&painter);
  engines.at(currentEngineIndex)->applyWatermark(&painter,QApplication::desktop()->availableGeometry());
  newFileName = _tempStorageDir + "/" + QString::number(QDateTime::currentDateTime().toTime_t()) +
                "." + format.toLower();
  newImage.save(newFileName,format.toAscii());

  return newFileName;
}

void Application::execPlay()
{
  qDebug() << "Application::execPlay()";

  playWasActive = true;

  trayIcon->setIcon(QIcon(":/images/idle"));

  playAction->setText(tr("Pause"));
  playAction->setIcon(QIcon(":/images/control_pause"));
  playAction->setData(true);
  disconnect(playAction,SIGNAL(triggered()),this,SLOT(execPlay()));
  connect(playAction,SIGNAL(triggered()),this,SLOT(execPause()));
  nextPhotoAction->setEnabled(true);

  if (settings.value(MAIN_SECTION).value(SWITCH_ON_PLAY).toBool())
    switchToNextState(ePlayTimeout);
  else
    timer->start();
}

void Application::execPause()
{
  qDebug() << "Application::execPause()";

  playWasActive = false;
  trayIcon->setIcon(QIcon(":/images/off"));

  playAction->setText(tr("Play"));
  playAction->setIcon(QIcon(":/images/control_play"));
  playAction->setData(false);
  disconnect(playAction,SIGNAL(triggered()),this,SLOT(execPause()));
  connect(playAction,SIGNAL(triggered()),this,SLOT(execPlay()));
  nextPhotoAction->setEnabled(false);

  timer->stop();
}

void Application::execAbout()
{
  AboutDialog aboutDialog;

  qDebug() << "Application::execAbout()";

  connect(&aboutDialog,SIGNAL(executed()),this,SLOT(deActivate()));
  connect(&aboutDialog,SIGNAL(closed()),this,SLOT(reActivate()));

  aboutDialog.exec();
}

void Application::execAboutQt()
{
  deActivate();

  qApp->aboutQt();

  reActivate();
}

void Application::execSavePhoto()
{
  QString fileName;

  qDebug() << "Application::execSavePhoto()";

  deActivate();

#ifdef Q_WS_X11
  fileName = QFileDialog::getSaveFileName(0,tr("Save photo"),QDir::homePath() + "/" + currentFile.fileName(),
                                          tr("Images (*.png *.xpm *.jpg)"),0,QFileDialog::DontUseNativeDialog);
#else
  fileName = QFileDialog::getSaveFileName(0,tr("Save photo"),
                                          QDesktopServices::storageLocation(QDesktopServices::PicturesLocation) + "/" + currentFile.fileName(),
                                          tr("Images (*.png *.xpm *.jpg)"),0,QFileDialog::DontUseNativeDialog);
#endif

  if (!fileName.isEmpty())
    QFile::copy(currentFile.absoluteFilePath(),fileName);

  reActivate();
}

void Application::execCancel()
{
  qDebug() << "Application::execCancel()";

  trayIcon->setIcon(QIcon(":/images/off"));
  trayIcon->setContextMenu(0);
  switchToNextState(eCancel);
}

void Application::execShowSettings()
{
  qDebug() << "Application::execShowSettings()";

  QMap<Engine::Core *, bool> activations;
  QListIterator<Engine::Core *> engine(engines);
  SettingsDialog dialog;

  connect(&dialog,SIGNAL(executed()),this,SLOT(deActivate()));
  connect(&dialog,SIGNAL(closed()),this,SLOT(reActivate()));
  connect(&dialog,SIGNAL(clearHistory()),this,SLOT(clearHistory()));

  dialog.loadSettings(settings);

  while (engine.hasNext())
  {
    engine.peekNext()->edit();
    dialog.addSettingsWidget(engine.next()->newSettingsWidget(&dialog));
  }

  engine.toFront();
  if (dialog.exec() == QDialog::Accepted)
  {
    Engine::Core *item;

    activations = dialog.activations();
    applySettings(dialog.settings());

    while (engine.hasNext())
    {
      item = engine.next();
      item->submit();
      item->setActive(activations.value(item));
    }

    saveSettings();

    checkPlayEnableable();
  }
  else
    while (engine.hasNext())
      engine.next()->revert();
}

void Application::execShowHistory()
{
  QNetworkProxy proxy;
  History::Dialog history(db,_tempStorageDir);

  dontSwitchState = true;

  qDebug() << "Application::execShowHistory()";

  connect(&history,SIGNAL(executed()),this,SLOT(deActivate()));
  connect(&history,SIGNAL(closed()),this,SLOT(reActivate()));

  connect(&history,SIGNAL(photoDownloaded(bool, const QFileInfo &)),
          this,SLOT(processDownloadedPhoto(bool, const QFileInfo &)));
  connect(&history,SIGNAL(infoCollected(const PhotoInfo &)),
          this,SLOT(processCollectedInfo(const PhotoInfo &)));
  connect(&history,SIGNAL(changePhoto(const QString &)),
          this,SLOT(showPhotoOnScreen(const QString &)));

  if (settings.value(NETWORK_SECTION).value(USE_PROXY).toBool())
  {
    proxy.setType(static_cast<QNetworkProxy::ProxyType> (settings.value(NETWORK_SECTION).value(PROXY_TYPE).toInt()));
    if (!settings.value(NETWORK_SECTION).value(USE_SYSTEM_PROXY).toBool())
    {
      proxy.setHostName(settings.value(NETWORK_SECTION).value(PROXY_SERVER).toString());
      proxy.setPort(settings.value(NETWORK_SECTION).value(PROXY_PORT).toInt());
    }
    else
      proxy.setHostName("0.0.0.0");
    if (settings.value(NETWORK_SECTION).value(PROXY_AUTHENTICATION).toBool())
    {
      proxy.setUser(settings.value(NETWORK_SECTION).value(PROXY_USERNAME).toString());
      proxy.setPassword(settings.value(NETWORK_SECTION).value(PROXY_PASSWORD).toString());
    }

    history.setProxy(proxy);
  }

  history.exec();

  dontSwitchState = false;
}

void Application::execGetExifInfo()
{
  qDebug() << "Application::execGetExifInfo()";

  Exif::InfoDialog dialog(currentFile);

  connect(&dialog,SIGNAL(executed()),this,SLOT(deActivate()));
  connect(&dialog,SIGNAL(closed()),this,SLOT(reActivate()));

  dialog.exec();
}

void Application::execNextPhoto()
{
  qDebug() << "Application::execNextPhoto()";

  timer->stop();
  switchToNextState(ePlayTimeout);
}

void Application::execExploreImageSource()
{
  QDesktopServices::openUrl(currentPhotoInfo.sourceUrl);
}

Version Application::currentVersion() const
{
  QSettings settings;

  settings.beginGroup(MAIN_SECTION);
  Version v(settings.value(VERSION,"0.0.0").toString());
  settings.endGroup();

  return v;
}

void Application::loadSettings()
{
  QListIterator<Engine::Core *> engine(engines);
  QSettings sourceSettings;

  qDebug() << "Application::loadSettings()";

  sourceSettings.beginGroup(MAIN_SECTION);
  settings[MAIN_SECTION][DISABLE_SPLASH_SCREEN] = sourceSettings.value(DISABLE_SPLASH_SCREEN,false);
  settings[MAIN_SECTION][QUIT_AFTER_BACKGROUND_CHANGE] = sourceSettings.value(QUIT_AFTER_BACKGROUND_CHANGE,false);
  settings[MAIN_SECTION][INTERVAL] = sourceSettings.value(INTERVAL,DEFAULT_INTERVAL);
  settings[MAIN_SECTION][INTERVAL_UNIT] = sourceSettings.value(INTERVAL_UNIT,MINUTES_UNIT);
  settings[MAIN_SECTION][BORDER_COLOR] = sourceSettings.value(BORDER_COLOR,QColor(Qt::black).name());
  settings[MAIN_SECTION][AUTO_COLOR] = sourceSettings.value(AUTO_COLOR,false);
  settings[MAIN_SECTION][WALLPAPER_POSITION] = sourceSettings.value(WALLPAPER_POSITION,static_cast<int> (Centered));
  settings[MAIN_SECTION][SWITCH_ON_PLAY] = sourceSettings.value(SWITCH_ON_PLAY,false);
  settings[MAIN_SECTION][PLAY_ON_START] = sourceSettings.value(PLAY_ON_START,false);
  settings[MAIN_SECTION][RUN_ON_SYS_START] = sourceSettings.value(RUN_ON_SYS_START,false);
  settings[MAIN_SECTION][ONLY_LANDSCAPES] = sourceSettings.value(ONLY_LANDSCAPES,false);
  settings[MAIN_SECTION][EXIF_ROTATE_IMAGES] = sourceSettings.value(EXIF_ROTATE_IMAGES,false);
  settings[MAIN_SECTION][SIZE_CONSTRAINT] = sourceSettings.value(SIZE_CONSTRAINT,0);
  settings[MAIN_SECTION][USE_FULL_DESKTOP_AREA] = sourceSettings.value(USE_FULL_DESKTOP_AREA,false);
  settings[MAIN_SECTION][VIEW_INFO_IN_TOOLTIP] = sourceSettings.value(VIEW_INFO_IN_TOOLTIP,false);
  settings[MAIN_SECTION][RANDOM_SEARCH] = sourceSettings.value(RANDOM_SEARCH,false);
  settings[MAIN_SECTION][VIEW_INFO_ON_PHOTO] = sourceSettings.value(VIEW_INFO_ON_PHOTO,false);
  settings[MAIN_SECTION][INFO_POSITION_ON_PHOTO] = sourceSettings.value(INFO_POSITION_ON_PHOTO,Qt::AlignRight);
  settings[MAIN_SECTION][MIN_FREE_DISK_SPACE] = sourceSettings.value(MIN_FREE_DISK_SPACE,200);
  settings[MAIN_SECTION][MIN_FREE_DISK_SPACE_FACTOR] = sourceSettings.value(MIN_FREE_DISK_SPACE_FACTOR,1024);
  settings[MAIN_SECTION][HISTORY_TIME_LIMIT] = sourceSettings.value(HISTORY_TIME_LIMIT,20);
  settings[MAIN_SECTION][HISTORY_TIME_LIMIT_FACTOR] = sourceSettings.value(HISTORY_TIME_LIMIT_FACTOR,0);

  currentEngineIndex = sourceSettings.value(LAST_USED_ENGINE,-1).toInt();
  sourceSettings.endGroup();

  sourceSettings.beginGroup(NETWORK_SECTION);
  settings[NETWORK_SECTION][USE_PROXY] = sourceSettings.value(USE_PROXY,false);
  settings[NETWORK_SECTION][USE_SYSTEM_PROXY] = sourceSettings.value(USE_SYSTEM_PROXY,true);
  settings[NETWORK_SECTION][PROXY_TYPE] = sourceSettings.value(PROXY_TYPE,QNetworkProxy::HttpProxy);
  settings[NETWORK_SECTION][PROXY_SERVER] = sourceSettings.value(PROXY_SERVER,QString());
  settings[NETWORK_SECTION][PROXY_PORT] = sourceSettings.value(PROXY_PORT,8080);
  settings[NETWORK_SECTION][PROXY_AUTHENTICATION] = sourceSettings.value(PROXY_AUTHENTICATION,false);
  settings[NETWORK_SECTION][PROXY_USERNAME] = sourceSettings.value(PROXY_USERNAME,QString());
  settings[NETWORK_SECTION][PROXY_PASSWORD] =
    QString(QByteArray::fromBase64(sourceSettings.value(PROXY_PASSWORD,QString()).toString().toAscii()));
  sourceSettings.endGroup();

  sourceSettings.beginGroup(ENGINES_SECTION);
  while (engine.hasNext())
  {
    sourceSettings.beginGroup(engine.peekNext()->name());
    engine.peekNext()->loadSettings(sourceSettings);
    engine.peekNext()->loadState(sourceSettings);
    sourceSettings.endGroup();
    engine.next();
  }
  sourceSettings.endGroup();

  applySettings(settings);

  saveSettings();
}

void Application::saveSettings()
{
  QListIterator<Engine::Core *> engine(engines);
  QSettings destSettings;

  qDebug() << "Application::saveSettings()";

  destSettings.beginGroup(MAIN_SECTION);
  destSettings.setValue(DISABLE_SPLASH_SCREEN,settings.value(MAIN_SECTION).value(DISABLE_SPLASH_SCREEN));
  destSettings.setValue(QUIT_AFTER_BACKGROUND_CHANGE,settings.value(MAIN_SECTION).value(QUIT_AFTER_BACKGROUND_CHANGE));
  destSettings.setValue(INTERVAL,settings.value(MAIN_SECTION).value(INTERVAL));
  destSettings.setValue(INTERVAL_UNIT,settings.value(MAIN_SECTION).value(INTERVAL_UNIT));
  destSettings.setValue(BORDER_COLOR,settings.value(MAIN_SECTION).value(BORDER_COLOR));
  destSettings.setValue(AUTO_COLOR,settings.value(MAIN_SECTION).value(AUTO_COLOR));
  destSettings.setValue(WALLPAPER_POSITION,settings.value(MAIN_SECTION).value(WALLPAPER_POSITION));
  destSettings.setValue(SWITCH_ON_PLAY,settings.value(MAIN_SECTION).value(SWITCH_ON_PLAY));
  destSettings.setValue(PLAY_ON_START,settings.value(MAIN_SECTION).value(PLAY_ON_START));
  destSettings.setValue(RUN_ON_SYS_START,settings.value(MAIN_SECTION).value(RUN_ON_SYS_START));
  destSettings.setValue(ONLY_LANDSCAPES,settings.value(MAIN_SECTION).value(ONLY_LANDSCAPES));
  destSettings.setValue(EXIF_ROTATE_IMAGES,settings.value(MAIN_SECTION).value(EXIF_ROTATE_IMAGES));
  destSettings.setValue(USE_FULL_DESKTOP_AREA,settings.value(MAIN_SECTION).value(USE_FULL_DESKTOP_AREA));
  destSettings.setValue(VIEW_INFO_IN_TOOLTIP,settings.value(MAIN_SECTION).value(VIEW_INFO_IN_TOOLTIP));
  destSettings.setValue(SIZE_CONSTRAINT,settings.value(MAIN_SECTION).value(SIZE_CONSTRAINT));
  destSettings.setValue(RANDOM_SEARCH,settings.value(MAIN_SECTION).value(RANDOM_SEARCH));
  destSettings.setValue(VIEW_INFO_ON_PHOTO,settings.value(MAIN_SECTION).value(VIEW_INFO_ON_PHOTO));
  destSettings.setValue(INFO_POSITION_ON_PHOTO,settings.value(MAIN_SECTION).value(INFO_POSITION_ON_PHOTO));
  destSettings.setValue(MIN_FREE_DISK_SPACE,settings.value(MAIN_SECTION).value(MIN_FREE_DISK_SPACE));
  destSettings.setValue(MIN_FREE_DISK_SPACE_FACTOR,settings.value(MAIN_SECTION).value(MIN_FREE_DISK_SPACE_FACTOR));
  destSettings.setValue(HISTORY_TIME_LIMIT,settings.value(MAIN_SECTION).value(HISTORY_TIME_LIMIT));
  destSettings.setValue(HISTORY_TIME_LIMIT_FACTOR,settings.value(MAIN_SECTION).value(HISTORY_TIME_LIMIT_FACTOR));
  destSettings.endGroup();

  destSettings.beginGroup(NETWORK_SECTION);
  destSettings.setValue(USE_PROXY,settings.value(NETWORK_SECTION).value(USE_PROXY));
  destSettings.setValue(USE_SYSTEM_PROXY,settings.value(NETWORK_SECTION).value(USE_SYSTEM_PROXY));
  destSettings.setValue(PROXY_TYPE,settings.value(NETWORK_SECTION).value(PROXY_TYPE));
  destSettings.setValue(PROXY_SERVER,settings.value(NETWORK_SECTION).value(PROXY_SERVER));
  destSettings.setValue(PROXY_PORT,settings.value(NETWORK_SECTION).value(PROXY_PORT));
  destSettings.setValue(PROXY_AUTHENTICATION,settings.value(NETWORK_SECTION).value(PROXY_AUTHENTICATION));
  destSettings.setValue(PROXY_USERNAME,settings.value(NETWORK_SECTION).value(PROXY_USERNAME));
  destSettings.setValue(PROXY_PASSWORD,
                        QString(settings.value(NETWORK_SECTION).value(PROXY_PASSWORD).toString().toAscii().toBase64()));
  destSettings.endGroup();

  destSettings.beginGroup(ENGINES_SECTION);
  while (engine.hasNext())
  {
    destSettings.beginGroup(engine.peekNext()->name());
    destSettings.remove(QString());
    engine.peekNext()->saveSettings(destSettings);
    engine.peekNext()->saveState(destSettings);
    destSettings.endGroup();
    engine.next();
  }
  destSettings.endGroup();
}

void Application::saveEnginesState()
{
  QSettings stateSettings;

  qDebug() << "Application::saveEnginesState()";

  stateSettings.beginGroup(MAIN_SECTION);
  stateSettings.setValue(LAST_USED_ENGINE,currentEngineIndex);
  stateSettings.endGroup();

  stateSettings.beginGroup(ENGINES_SECTION);

  stateSettings.beginGroup(engines.at(currentEngineIndex)->name());
  engines.at(currentEngineIndex)->saveState(stateSettings);
  stateSettings.endGroup();

  stateSettings.endGroup();
}

void Application::applySettings(WallySettings newSettings)
{
  QListIterator<Engine::Core *> engine(engines);
  QNetworkProxy proxy(QNetworkProxy::NoProxy);

  qDebug() << "Application::applySettings(...)";

  settings = newSettings;

  #ifdef Q_WS_WIN
    QSettings regSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                          QSettings::NativeFormat);

    if (settings.value(MAIN_SECTION).value(RUN_ON_SYS_START).toBool())
      regSettings.setValue(APPLICATION_NAME,QDir::toNativeSeparators(qApp->applicationFilePath()));
    else
      regSettings.remove(APPLICATION_NAME);
  #endif

  switch (settings.value(MAIN_SECTION).value(INTERVAL_UNIT).toInt())
  {
    case SECONDS_UNIT:
      timer->setInterval(settings.value(MAIN_SECTION).value(INTERVAL).toInt() * 1000);
      break;

    default:
    case MINUTES_UNIT:
      timer->setInterval(settings.value(MAIN_SECTION).value(INTERVAL).toInt() * 60000);
      break;

    case HOURS_UNIT:
      timer->setInterval(settings.value(MAIN_SECTION).value(INTERVAL).toInt() * 3600000);
      break;
  }

  if (settings.value(NETWORK_SECTION).value(USE_PROXY).toBool())
  {
    proxy.setType(static_cast<QNetworkProxy::ProxyType> (settings.value(NETWORK_SECTION).value(PROXY_TYPE).toInt()));
    if (!settings.value(NETWORK_SECTION).value(USE_SYSTEM_PROXY).toBool())
    {
      proxy.setHostName(settings.value(NETWORK_SECTION).value(PROXY_SERVER).toString());
      proxy.setPort(settings.value(NETWORK_SECTION).value(PROXY_PORT).toInt());
    }
    else
      proxy.setHostName("0.0.0.0");
    if (settings.value(NETWORK_SECTION).value(PROXY_AUTHENTICATION).toBool())
    {
      proxy.setUser(settings.value(NETWORK_SECTION).value(PROXY_USERNAME).toString());
      proxy.setPassword(settings.value(NETWORK_SECTION).value(PROXY_PASSWORD).toString());
    }

    qDebug() << "\tnew proxy is"
             << proxy.user().toAscii().constData() << "@"
             << proxy.hostName().toAscii().constData() << ":"
             << QString::number(proxy.port()).toAscii().constData();
  }

  while (engine.hasNext())
  {
    HttpEngine::Core *httpEngine = qobject_cast<HttpEngine::Core *> (engine.next());

    if (httpEngine)
    {
      httpEngine->setProxy(proxy);
      httpEngine->setFreeDiskSpaceThreshold(settings.value(MAIN_SECTION).value(MIN_FREE_DISK_SPACE).toInt() *
                                            settings.value(MAIN_SECTION).value(MIN_FREE_DISK_SPACE_FACTOR).toInt() * 1024);
    }
  }
}

void Application::addEngine(Engine::Core *e)
{
  qDebug() << "Application::addEngine(" << e->name().toAscii().constData() << ")";

  engines << e;
}

void Application::checkPlayEnableable()
{
  qDebug() << "Application::checkPlayEnableable()";

  QListIterator<Engine::Core *> engine(engines);
  int photosCount = 0;

  while (engine.hasNext())
    photosCount += engine.next()->rowCount();

  if (!photosCount)
    execPause();
  playAction->setEnabled(photosCount);
}

void Application::setupBeforeLaunch()
{
  qDebug() << "Application::setupBeforeLaunch()";

  QSettings versionSettings;

  versionSettings.beginGroup(MAIN_SECTION);
  versionSettings.setValue(VERSION,Version(QString(APPLICATION_VERSION)).toString());
  versionSettings.endGroup();

  checkPlayEnableable();

  if (playAction->isEnabled() && settings.value(MAIN_SECTION).value(PLAY_ON_START).toBool())
    QTimer::singleShot(5000,playAction,SLOT(trigger()));
}

#ifdef Q_WS_X11
bool Application::x11EventFilter(XEvent *event)
{
  XKeyEvent *ev;

  if (captureKeymaps && (event->type == KeyPress))
  {
    ev = reinterpret_cast<XKeyEvent *> (event);

    if ((ev->state & ControlMask) &&
        (ev->state & ShiftMask))
      switch (XLookupKeysym(ev,0))
      {
        case XK_a:
          qDebug() << "Application::x11EventFilter(...) captured CTRL+Shift+A";
          aboutAction->trigger();
          break;

        case XK_s:
          qDebug() << "Application::x11EventFilter(...) captured CTRL+Shift+S";
          configureAction->trigger();
          break;

        case XK_n:
          qDebug() << "Application::x11EventFilter(...) captured CTRL+Shift+N";
          nextPhotoAction->trigger();
          break;

        case XK_q:
          qDebug() << "Application::x11EventFilter(...) captured CTRL+Shift+Q";
          quitAction->trigger();
          break;

        case XK_d:
          qDebug() << "Application::x11EventFilter(...) captured CTRL+Shift+D";
          savePhotoAction->trigger();
          break;

        case XK_i:
          qDebug() << "Application::x11EventFilter(...) captured CTRL+Shift+I";
          getExifInfoAction->trigger();
          break;

        case XK_h:
          qDebug() << "Application::x11EventFilter(...) captured CTRL+Shift+H";
          historyAction->trigger();
          break;

        case XK_w:
          qDebug() << "Application::x11EventFilter(...) captured CTRL+Shift+W";
          viewSourceAction->trigger();
          break;

        case XK_p:
          qDebug() << "Application::x11EventFilter(...) captured CTRL+Shift+P";
          playAction->trigger();
          break;

        default:
          break;
      }
  }

  XAllowEvents(QX11Info::display(),ReplayKeyboard,event->xkey.time);
  XFlush(QX11Info::display());

  return false;
}
#endif

#ifdef Q_WS_WIN
bool Application::winEventFilter(MSG *msg, long * /* result */)
{
  if ((msg->message == WM_USER) && (msg->wParam == 1234) && (msg->lParam == 5678))
  {
    qDebug() << "Application::winEventFilter(WM_USER,1234,5678)";

    cancelAction->trigger();
    QTimer::singleShot(250,this,SLOT(quit()));
  }

  return false;
}
#endif

void Application::deActivate()
{
  playTimerWasActive = timer->isActive();

  trayIcon->setIcon(QIcon(":/images/off"));
  trayIcon->setContextMenu(0);
  timer->stop();
}

void Application::reActivate()
{
  trayIcon->setContextMenu(contextMenu);

  if (playTimerWasActive)
  {
    trayIcon->setIcon(QIcon(":/images/idle"));
    timer->start();
  }
}

void Application::showTrayMessage()
{
  trayIcon->showMessage(QString("%1 %2").arg(APPLICATION_NAME).arg(APPLICATION_VERSION),
                        tr("Right-click to show main menu"));
}
