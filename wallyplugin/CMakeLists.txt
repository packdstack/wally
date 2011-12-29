cmake_minimum_required(VERSION 2.6)

project(wallyplugin)

set(CMAKE_BUILD_TYPE Release)
execute_process(COMMAND "kde4-config" "--prefix" OUTPUT_VARIABLE KDE_CUSTOM_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CMAKE_INSTALL_PREFIX "${KDE_CUSTOM_PREFIX}")

set(QT_USE_QTDBUS TRUE)

find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES} ../include)

set(wallyplugin_SRCS wallyplugin.cpp wallyplugin.h)

kde4_add_plugin(plasma_wallpaper_wallyplugin ${wallyplugin_SRCS})
target_link_libraries(plasma_wallpaper_wallyplugin ${QT_LIBRARIES} ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS})
if(CMAKE_BUILD_TYPE STREQUAL Release)
  get_property( WALLY_PLUGIN_PATH TARGET plasma_wallpaper_wallyplugin PROPERTY LOCATION )
  add_custom_command( TARGET plasma_wallpaper_wallyplugin POST_BUILD COMMAND strip ARGS ${WALLY_PLUGIN_PATH} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
endif(CMAKE_BUILD_TYPE STREQUAL Release)

install(TARGETS plasma_wallpaper_wallyplugin DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-wallpaper-wallyplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES wallyplugin.png DESTINATION ${ICON_INSTALL_DIR}/oxygen/16x16/apps)

install(CODE "execute_process(COMMAND kbuildsycoca4 OUTPUT_QUIET ERROR_QUIET)")
install(CODE "execute_process(COMMAND kdialog --title \"WallyPlugin\" --msgbox \"WallyPlugin will be loaded next time KDE is started. Please select it in Appearance Settings.\")")
