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

#ifndef DEFS_H
#define DEFS_H

#include <QtCore>

#define TEMP_PATH ".wally"
#define MAX_PIXEL_NUMBER 9000000

#define MINUTES_UNIT 0
#define HOURS_UNIT 1
#define SECONDS_UNIT 2

#define MAIN_SECTION "Main"
#define DISABLE_SPLASH_SCREEN "disableSplashScreen"
#define QUIT_AFTER_BACKGROUND_CHANGE "quitAfterBackgroundChange"
#define RANDOM_SEARCH "randomSearch"
#define SIZE_CONSTRAINT "sizeConstraint"
#define INTERVAL "interval"
#define INTERVAL_UNIT "intervalUnit"
#define BORDER_COLOR "borderColor"
#define AUTO_COLOR "autoColor"
#define WALLPAPER_POSITION "wallpaperPosition"
#define SWITCH_ON_PLAY "switchOnPlay"
#define PLAY_ON_START "playOnStart"
#define RUN_ON_SYS_START "runOnSysStart"
#define ONLY_LANDSCAPES "onlyLandscapes"
#define EXIF_ROTATE_IMAGES "exifRotateImages"
#define VIEW_INFO_IN_TOOLTIP "viewInfoInTooltip"
#define VIEW_INFO_ON_PHOTO "viewInfoOnPhoto"
#define INFO_POSITION_ON_PHOTO "infoPositionOnPhoto"
#define USE_FULL_DESKTOP_AREA "useFullDesktopArea"
#define LOCALE "locale"
#define LAST_USED_ENGINE "lastUsedEngine"
#define MIN_FREE_DISK_SPACE "minFreeDiskSpace"
#define MIN_FREE_DISK_SPACE_FACTOR "minFreeDiskSpaceFactor"
#define HISTORY_TIME_LIMIT "historyTimeLimit"
#define HISTORY_TIME_LIMIT_FACTOR "historyTimeLimitFactor"
#define VERSION "version"

#define NETWORK_SECTION "Network"
#define USE_PROXY "useProxy"
#define USE_SYSTEM_PROXY "useSystemProxy"
#define PROXY_TYPE "proxyType"
#define PROXY_SERVER "proxyServer"
#define PROXY_PORT "proxyPort"
#define PROXY_AUTHENTICATION "proxyAuthentication"
#define PROXY_USERNAME "proxyUsername"
#define PROXY_PASSWORD "proxyPassword"

typedef QMap<QString, QMap<QString, QVariant> > WallySettings;

struct PhotoInfo
{
  QString owner;
  QString title;
  QString description;
  QString location;
  QUrl sourceUrl;
  QString searchString;
};

#endif
