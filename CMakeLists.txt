cmake_minimum_required(VERSION 2.6)

if( COMMAND cmake_policy )
  if( POLICY CMP0017 )
    cmake_policy( SET CMP0017 NEW )
  endif( POLICY CMP0017 )
endif( COMMAND cmake_policy )

project( wally )

if( STATIC )
  if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Release )
  endif( NOT CMAKE_BUILD_TYPE )

  add_definitions( -DQT_STATICPLUGIN -DWALLY_STATIC )
endif( STATIC )

set( WALLY_SRCS
       include/version.h
       include/idcache.h
       src/json.cpp
       include/json.h
       src/diskinfo.cpp
       include/diskinfo.h
       src/exif.cpp
       include/exif.h
       include/networkmanager.h
       include/networkmanager_p.h
       src/networkmanager.cpp
       src/gui.cpp
       include/gui.h
       src/mapviewer.cpp
       include/mapviewer.h
       src/engine.cpp
       include/engine.h
       src/xtengine.cpp
       include/xtengine.h
       src/httpengine.cpp
       include/httpengine.h
       src/files.cpp
       include/files.h
       src/folders.cpp
       include/folders.h
       src/flickr.cpp
       include/flickr.h
       src/yahoo.cpp
       include/yahoo.h
       src/panoramio.cpp
       include/panoramio.h
#       src/pikeo.cpp
#       include/pikeo.h
       src/photobucket.cpp
       include/photobucket.h
       src/ipernity.cpp
       include/ipernity.h
       src/buzznet.cpp
       include/buzznet.h
       src/picasa.cpp
       include/picasa.h
       src/smugmug.cpp
       include/smugmug.h
#       src/bing.cpp
#       include/bing.h
       src/google.cpp
       include/google.h
       src/vladstudio.cpp
       include/vladstudio.h
       src/deviantart.cpp
       include/deviantart.h
       src/utils.cpp
       include/utils.h
       src/splash.cpp
       include/splash.h
       src/settings.cpp
       include/settings.h
       src/history.cpp
       include/history.h
       src/viewer.cpp
       include/viewer.h
       src/about.cpp
       include/about.h
       src/wally.cpp
       include/wally.h
       src/main.cpp
       include/defs.h
   )

set( WALLY_MOC_HDRS
       include/exif.h
       include/networkmanager.h
       include/networkmanager_p.h
       include/gui.h
       include/mapviewer.h
       include/engine.h
       include/xtengine.h
       include/httpengine.h
       include/files.h
       include/folders.h
       include/flickr.h
       include/yahoo.h
       include/panoramio.h
#       include/pikeo.h
       include/photobucket.h
       include/ipernity.h
       include/buzznet.h
       include/picasa.h
       include/smugmug.h
#       include/bing.h
       include/google.h
       include/vladstudio.h
       include/deviantart.h
       include/splash.h
       include/settings.h
       include/history.h
       include/viewer.h
       include/about.h
       include/wally.h
   )

set( WALLY_UIS
       ui/files.ui
       ui/folders.ui
       ui/http.ui
       ui/settings.ui
       ui/viewer.ui
       ui/about.ui
   )

set( WALLY_IMAGES_RCS
       res/images.qrc
   )

set( WALLY_LANGUAGES_RCS
       ${CMAKE_CURRENT_BINARY_DIR}/languages.qrc
   )

set( WALLY_TS
       res/lang/wally_ca.ts
       res/lang/wally_cz.ts
       res/lang/wally_de.ts
       res/lang/wally_es.ts
       res/lang/wally_fr.ts
       res/lang/wally_gr.ts
       res/lang/wally_it.ts
       res/lang/wally_kr.ts
       res/lang/wally_pl.ts
       res/lang/wally_pt_BR.ts
       res/lang/wally_ru.ts
       res/lang/wally_zh_CN.ts
       res/lang/wally_hu.ts
       res/lang/wally_dk.ts
       res/lang/wally_se.ts
       res/lang/wally_tr.ts
   )

add_definitions( -Wall -Wextra -fexceptions -frtti -DUNICODE -DQT_THREAD_SUPPORT )

set( QT_USE_QTNETWORK TRUE )
set( QT_USE_QTXML TRUE )
set( QT_USE_QTSVG TRUE )
if ( ( NOT APPLE ) AND ( NOT WIN32 ) )
  set( QT_USE_QTDBUS TRUE )
endif( ( NOT APPLE ) AND ( NOT WIN32 ) )
set( QT_USE_QTSQL TRUE )

find_package( PkgConfig REQUIRED )
pkg_check_modules( EXIF libexif )
if( EXIF_FOUND )
  add_definitions( -DEXIF )
  link_directories( ${EXIF_LIBRARY_DIRS} )
endif( EXIF_FOUND )

find_package( Qt4 REQUIRED )
include( ${QT_USE_FILE} )
configure_file( res/languages.qrc ${CMAKE_CURRENT_BINARY_DIR}/languages.qrc )
qt4_add_translation( WALLY_QMS ${WALLY_TS} )
qt4_add_resources( WALLY_IMAGES_RC_SRCS ${WALLY_IMAGES_RCS} )
qt4_add_resources( WALLY_LANGUAGES_RC_SRCS ${WALLY_LANGUAGES_RCS} )
qt4_wrap_ui( WALLY_UI_HDRS ${WALLY_UIS} )
qt4_wrap_cpp( WALLY_MOC_SRCS ${WALLY_MOC_HDRS} )
link_directories( ${QT_LIBRARY_DIR} ${QT_PLUGINS_DIR}/imageformats ${QT_PLUGINS_DIR}/sqldrivers )

include_directories( include ${EXIF_INCLUDE_DIRS} ${CMAKE_BINARY_DIR} )

if( APPLE )

  set( PROGNAME Wally )
  set( CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.4u.sdk/" )
  set( CMAKE_OSX_ARCHITECTURES "ppc;i386" )
  set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.4 )

  find_library( QT_GIF_PLUGIN qgif PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats )
  find_library( QT_ICO_PLUGIN qico PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats )
  find_library( QT_JPEG_PLUGIN qjpeg PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats )
  find_library( QT_MNG_PLUGIN qmng PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats )
  find_library( QT_SVG_PLUGIN qsvg PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats )
  find_library( QT_TIFF_PLUGIN qtiff PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats )
  find_library( QT_SQLITE_PLUGIN qsqlite PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES sqldrivers )

  find_library( SYSTEM_CONFIGURATION_LIBRARY SystemConfiguration )

  if( EXIF_FOUND )
    find_library( ICONV libiconv )
    if( ICONV-NOTFOUND )
      message( FATAL_ERROR "libiconv not found" )
    endif( ICONV-NOTFOUND )

    find_library( INTL libintl )
    if( INTL-NOTFOUND )
      message( FATAL_ERROR "gettext not found" )
    endif( INTL-NOTFOUND )
  endif( EXIF_FOUND )

  add_executable( ${PROGNAME} MACOSX_BUNDLE ${WALLY_QMS} ${WALLY_SRCS} ${WALLY_MOC_SRCS} ${WALLY_IMAGES_RC_SRCS} ${WALLY_IMAGES_RC_SRCS}
                                            ${WALLY_LANGUAGES_RC_SRCS} ${WALLY_UI_HDRS} ${CMAKE_CURRENT_SOURCE_DIR}/mac/wally.icns )
  set_source_files_properties ( ${CMAKE_CURRENT_SOURCE_DIR}/mac/wally.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
  target_link_libraries( ${PROGNAME} ${QT_GIF_PLUGIN} ${QT_ICO_PLUGIN} ${QT_JPEG_PLUGIN} ${QT_MNG_PLUGIN}
                                     ${QT_SVG_PLUGIN} ${QT_TIFF_PLUGIN} ${QT_SQLITE_PLUGIN}
                                     ${QT_LIBRARIES} -lz ${EXIF_LIBRARIES} -lqgif -lqico -lqjpeg -lqmng -lqsvg -lqtiff -lqsqlite
                                     -liconv -lintl ${SYSTEM_CONFIGURATION_LIBRARY} )

  set_target_properties( ${PROGNAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/mac/Info.plist )

  if( CMAKE_BUILD_TYPE STREQUAL Release )
    get_property( WALLY_PATH TARGET ${PROGNAME} PROPERTY LOCATION )
    add_custom_command( TARGET ${PROGNAME} POST_BUILD COMMAND strip ARGS ${WALLY_PATH} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
  endif( CMAKE_BUILD_TYPE STREQUAL Release )

  if( COMPRESS )
    find_program( UPX upx )

    if( UPX )
      get_property( WALLY_PATH TARGET ${PROGNAME} PROPERTY LOCATION )
      add_custom_command( TARGET ${PROGNAME} POST_BUILD COMMAND ${UPX} ARGS ${WALLY_PATH} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
    endif( UPX )
  endif( COMPRESS )

elseif( WIN32 )

  set( PROGNAME Wally )
  add_definitions( -DWINVER=0x0500 )
  add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wally_rc.o
                      COMMAND windres.exe -I.
                                          -o ${CMAKE_CURRENT_BINARY_DIR}/wally_rc.o
                                          -i res/wally.rc
                      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
  set( WALLY_SRCS ${WALLY_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/wally_rc.o )

  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -enable-auto-import" )

  if( STATIC )
    set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static" )
    find_library( QT_GIF_PLUGIN qgif PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats )
    find_library( QT_ICO_PLUGIN qico PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats )
    find_library( QT_JPEG_PLUGIN qjpeg PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats )
    find_library( QT_MNG_PLUGIN qmng PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats )
    find_library( QT_SVG_PLUGIN qsvg PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats )
    find_library( QT_TIFF_PLUGIN qtiff PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES imageformats )
    find_library( QT_SQLITE_PLUGIN qsqlite PATHS ${QT_PLUGINS_DIR} PATH_SUFFIXES sqldrivers )
  endif( STATIC )

  add_executable( ${PROGNAME} WIN32 ${WALLY_QMS} ${WALLY_SRCS} ${WALLY_MOC_SRCS} ${WALLY_IMAGES_RC_SRCS} ${WALLY_LANGUAGES_RC_SRCS} ${WALLY_UI_HDRS} )

  if( STATIC )
   target_link_libraries( ${PROGNAME} ${QT_GIF_PLUGIN} ${QT_ICO_PLUGIN} ${QT_JPEG_PLUGIN}
                                      ${QT_MNG_PLUGIN} ${QT_SVG_PLUGIN} ${QT_TIFF_PLUGIN} ${QT_SQLITE_PLUGIN}
                                      ${QT_LIBRARIES} ${EXIF_LIBRARIES} -lshlwapi -lversion -lshell32 )
  else( STATIC )
    target_link_libraries( ${PROGNAME} ${QT_LIBRARIES} ${EXIF_LIBRARIES} )
  endif( STATIC )

  if( COMPRESS )
    find_program( UPX upx )

    if( UPX )
      get_property( WALLY_PATH TARGET ${PROGNAME} PROPERTY LOCATION )
      add_custom_command( TARGET ${PROGNAME} POST_BUILD COMMAND ${UPX} ARGS ${WALLY_PATH} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
    endif( UPX )
  endif( COMPRESS )

else( APPLE )

  set( PROGNAME wally )

  add_executable( ${PROGNAME} ${WALLY_QMS} ${WALLY_SRCS} ${WALLY_MOC_SRCS} ${WALLY_IMAGES_RC_SRCS} ${WALLY_LANGUAGES_RC_SRCS} ${WALLY_UI_HDRS} )
  target_link_libraries( ${PROGNAME} ${QT_LIBRARIES} ${EXIF_LIBRARIES} )
  install( TARGETS ${PROGNAME} DESTINATION /usr/bin )

  if( CMAKE_BUILD_TYPE STREQUAL Release )
    get_property( WALLY_PATH TARGET ${PROGNAME} PROPERTY LOCATION )
    add_custom_command( TARGET ${PROGNAME} POST_BUILD COMMAND strip ARGS ${WALLY_PATH} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
  endif( CMAKE_BUILD_TYPE STREQUAL Release )

  find_package( KDE4 )
  if ( KDE4_FOUND )
    add_subdirectory( ${CMAKE_SOURCE_DIR}/wallyplugin )
  endif( KDE4_FOUND )

endif( APPLE )
