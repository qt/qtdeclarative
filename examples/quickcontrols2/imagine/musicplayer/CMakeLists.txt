# Generated from musicplayer.pro.

cmake_minimum_required(VERSION 3.14)
project(musicplayer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols2/imagine/musicplayer")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS QuickControls2)

qt_add_executable(musicplayer
    musicplayer.cpp
)
set_target_properties(musicplayer PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(musicplayer PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickControls2
)


# Resources:
set(icons_resource_files
    "musicplayer/32x32/bluetooth.png"
    "musicplayer/32x32/cart.png"
    "musicplayer/32x32/cloud.png"
    "musicplayer/32x32/favorite.png"
    "musicplayer/32x32/filter.png"
    "musicplayer/32x32/folder.png"
    "musicplayer/32x32/message.png"
    "musicplayer/32x32/music.png"
    "musicplayer/32x32/next.png"
    "musicplayer/32x32/pause.png"
    "musicplayer/32x32/power.png"
    "musicplayer/32x32/previous.png"
    "musicplayer/32x32/repeat.png"
    "musicplayer/32x32/save.png"
    "musicplayer/32x32/settings.png"
    "musicplayer/32x32/shuffle.png"
    "musicplayer/32x32/stop.png"
    "musicplayer/32x32@2/bluetooth.png"
    "musicplayer/32x32@2/cart.png"
    "musicplayer/32x32@2/cloud.png"
    "musicplayer/32x32@2/favorite.png"
    "musicplayer/32x32@2/filter.png"
    "musicplayer/32x32@2/folder.png"
    "musicplayer/32x32@2/grid.png"
    "musicplayer/32x32@2/message.png"
    "musicplayer/32x32@2/music.png"
    "musicplayer/32x32@2/next.png"
    "musicplayer/32x32@2/pause.png"
    "musicplayer/32x32@2/power.png"
    "musicplayer/32x32@2/previous.png"
    "musicplayer/32x32@2/repeat.png"
    "musicplayer/32x32@2/save.png"
    "musicplayer/32x32@2/settings.png"
    "musicplayer/32x32@2/shuffle.png"
    "musicplayer/32x32@2/stop.png"
    "musicplayer/index.theme"
)

qt6_add_resources(musicplayer "icons"
    PREFIX
        "/icons"
    BASE
        "icons"
    FILES
        ${icons_resource_files}
)
set(imagine-assets_resource_files
    "applicationwindow-background.png"
    "button-background-checked-hovered.9.png"
    "button-background-checked-hovered@2x.9.png"
    "button-background-checked.9.png"
    "button-background-checked@2x.9.png"
    "button-background-disabled.9.png"
    "button-background-disabled@2x.9.png"
    "button-background-hovered.9.png"
    "button-background-hovered@2x.9.png"
    "button-background-pressed.9.png"
    "button-background-pressed@2x.9.png"
    "button-background.9.png"
    "button-background@2x.9.png"
    "combobox-background-hovered.9.png"
    "combobox-background-hovered@2x.9.png"
    "combobox-background-open.9.png"
    "combobox-background-open@2x.9.png"
    "combobox-background-pressed.9.png"
    "combobox-background-pressed@2x.9.png"
    "combobox-background.9.png"
    "combobox-background@2x.9.png"
    "combobox-indicator-hovered.png"
    "combobox-indicator-hovered@2x.png"
    "combobox-indicator-open.png"
    "combobox-indicator-open@2x.png"
    "combobox-indicator-pressed.png"
    "combobox-indicator-pressed@2x.png"
    "combobox-indicator.png"
    "combobox-indicator@2x.png"
    "combobox-popup.9.png"
    "combobox-popup@2x.9.png"
    "dial-background-hovered.png"
    "dial-background-hovered@2x.png"
    "dial-background-pressed.png"
    "dial-background-pressed@2x.png"
    "dial-background.png"
    "dial-background@2x.png"
    "dial-handle-pressed.png"
    "dial-handle-pressed@2x.png"
    "dial-handle.png"
    "dial-handle@2x.png"
    "frame-background.9.png"
    "frame-background@2x.9.png"
    "itemdelegate-background-checked.9.png"
    "itemdelegate-background-checked@2x.9.png"
    "itemdelegate-background-disabled.9.png"
    "itemdelegate-background-disabled@2x.9.png"
    "itemdelegate-background-hovered.9.png"
    "itemdelegate-background-hovered@2x.9.png"
    "itemdelegate-background-pressed.9.png"
    "itemdelegate-background-pressed@2x.9.png"
    "itemdelegate-background.9.png"
    "itemdelegate-background@2x.9.png"
    "roundbutton-background-checked-hovered.png"
    "roundbutton-background-checked-hovered@2x.png"
    "roundbutton-background-checked.png"
    "roundbutton-background-checked@2x.png"
    "roundbutton-background-disabled.png"
    "roundbutton-background-disabled@2x.png"
    "roundbutton-background-hovered.png"
    "roundbutton-background-hovered@2x.png"
    "roundbutton-background-pressed.png"
    "roundbutton-background-pressed@2x.png"
    "roundbutton-background.png"
    "roundbutton-background@2x.png"
    "scrollbar-handle-interactive-hovered.png"
    "scrollbar-handle-interactive-hovered@2x.png"
    "scrollbar-handle-interactive-pressed.png"
    "scrollbar-handle-interactive-pressed@2x.png"
    "scrollbar-handle-interactive.png"
    "scrollbar-handle-interactive@2x.png"
    "slider-background-horizontal-disabled.9.png"
    "slider-background-horizontal-disabled@2x.9.png"
    "slider-background-horizontal.9.png"
    "slider-background-horizontal@2x.9.png"
    "slider-background-vertical-disabled.9.png"
    "slider-background-vertical-disabled@2x.9.png"
    "slider-background-vertical.9.png"
    "slider-background-vertical@2x.9.png"
    "slider-handle-disabled.png"
    "slider-handle-disabled@2x.png"
    "slider-handle-hovered.png"
    "slider-handle-hovered@2x.png"
    "slider-handle.png"
    "slider-handle@2x.png"
    "slider-progress-horizontal.9.png"
    "slider-progress-horizontal@2x.9.png"
    "slider-progress-vertical-disabled.9.png"
    "slider-progress-vertical-disabled@2x.9.png"
    "slider-progress-vertical.9.png"
    "slider-progress-vertical@2x.9.png"
    "textfield-background-disabled.9.png"
    "textfield-background-disabled@2x.9.png"
    "textfield-background.9.png"
    "textfield-background@2x.9.png"
    "toolbar-background.9.png"
    "toolbar-background@2x.9.png"
    "toolbutton-background-checked-hovered.9.png"
    "toolbutton-background-checked-hovered@2x.9.png"
    "toolbutton-background-checked.9.png"
    "toolbutton-background-checked@2x.9.png"
    "toolbutton-background-hovered.9.png"
    "toolbutton-background-hovered@2x.9.png"
    "toolbutton-background-pressed.9.png"
    "toolbutton-background-pressed@2x.9.png"
    "toolbutton-background.9.png"
    "toolbutton-background@2x.9.png"
    "tooltip-background.9.png"
    "tooltip-background@2x.9.png"
)

qt6_add_resources(musicplayer "imagine-assets"
    PREFIX
        "/imagine-assets"
    BASE
        "imagine-assets"
    FILES
        ${imagine-assets_resource_files}
)
set(qmake_immediate_resource_files
    "images/album-cover.jpg"
    "musicplayer.qml"
    "qtquickcontrols2.conf"
)

qt6_add_resources(musicplayer "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

install(TARGETS musicplayer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
