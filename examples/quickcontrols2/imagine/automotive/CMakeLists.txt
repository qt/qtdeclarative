# Generated from automotive.pro.

cmake_minimum_required(VERSION 3.14)
project(automotive LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols2/imagine/automotive")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS QuickControls2)

qt_add_executable(automotive
    automotive.cpp
)
set_target_properties(automotive PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(automotive PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickControls2
)


# Resources:
set(icons_resource_files
    "automotive/44x44/air-con.png"
    "automotive/44x44/command.png"
    "automotive/44x44/message.png"
    "automotive/44x44/music.png"
    "automotive/44x44/seats.png"
    "automotive/44x44/settings.png"
    "automotive/44x44/statistics.png"
    "automotive/44x44/windows.png"
    "automotive/44x44@2/air-con.png"
    "automotive/44x44@2/command.png"
    "automotive/44x44@2/message.png"
    "automotive/44x44@2/music.png"
    "automotive/44x44@2/navigation.png"
    "automotive/44x44@2/seats.png"
    "automotive/44x44@2/settings.png"
    "automotive/44x44@2/statistics.png"
    "automotive/44x44@2/windows.png"
    "automotive/icons.svg"
    "automotive/index.theme"
    "car.png"
    "car@2x.png"
    "warning.png"
    "warning@2x.png"
    "weather.png"
    "weather@2x.png"
)

qt6_add_resources(automotive "icons"
    PREFIX
        "/icons"
    BASE
        "icons"
    FILES
        ${icons_resource_files}
)
set(imagine-assets_resource_files
    "applicationwindow-background.png"
    "applicationwindow-background@2x.png"
    "button-background-checked-hovered.9.png"
    "button-background-checked-hovered@2x.9.png"
    "button-background-checked.9.png"
    "button-background-checked@2x.9.png"
    "button-background-hovered.9.png"
    "button-background-hovered@2x.9.png"
    "button-background-pressed.9.png"
    "button-background-pressed@2x.9.png"
    "button-background.9.png"
    "button-background@2x.9.png"
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
    "itemdelegate-background-hovered.9.png"
    "itemdelegate-background-hovered@2x.9.png"
    "itemdelegate-background-pressed.9.png"
    "itemdelegate-background-pressed@2x.9.png"
    "itemdelegate-background.9.png"
    "itemdelegate-background@2x.9.png"
    "radiobutton-indicator-checked-hovered.png"
    "radiobutton-indicator-checked-hovered@2x.png"
    "radiobutton-indicator-checked-pressed.png"
    "radiobutton-indicator-checked-pressed@2x.png"
    "radiobutton-indicator-checked.png"
    "radiobutton-indicator-checked@2x.png"
    "radiobutton-indicator-hovered.png"
    "radiobutton-indicator-hovered@2x.png"
    "radiobutton-indicator-pressed.png"
    "radiobutton-indicator-pressed@2x.png"
    "radiobutton-indicator.png"
    "radiobutton-indicator@2x.png"
    "scrollindicator-handle.png"
    "scrollindicator-handle@2x.png"
    "slider-background-horizontal.9.png"
    "slider-background-horizontal@2x.9.png"
    "slider-handle-hovered.png"
    "slider-handle-hovered@2x.png"
    "slider-handle-pressed.png"
    "slider-handle-pressed@2x.png"
    "slider-handle.png"
    "slider-handle@2x.png"
    "slider-progress-horizontal-pressed.9.png"
    "slider-progress-horizontal-pressed@2x.9.png"
    "slider-progress-horizontal.9.png"
    "slider-progress-horizontal@2x.9.png"
    "switchdelegate-background.9.png"
    "switchdelegate-background@2x.9.png"
    "switchdelegate-handle-checked-hovered.png"
    "switchdelegate-handle-checked-hovered@2x.png"
    "switchdelegate-handle-checked.png"
    "switchdelegate-handle-checked@2x.png"
    "switchdelegate-handle-hovered.png"
    "switchdelegate-handle-hovered@2x.png"
    "switchdelegate-handle-pressed.png"
    "switchdelegate-handle-pressed@2x.png"
    "switchdelegate-handle.png"
    "switchdelegate-handle@2x.png"
    "switchdelegate-indicator-pressed.png"
    "switchdelegate-indicator-pressed@2x.png"
    "switchdelegate-indicator.png"
    "switchdelegate-indicator@2x.png"
    "toolseparator-separator-vertical.9.png"
    "toolseparator-separator-vertical@2x.9.png"
)

qt6_add_resources(automotive "imagine-assets"
    PREFIX
        "/imagine-assets"
    BASE
        "imagine-assets"
    FILES
        ${imagine-assets_resource_files}
)
set(qml_resource_files
    "FeatureButton.qml"
    "LargeLabel.qml"
    "automotive.qml"
)

qt6_add_resources(automotive "qml"
    PREFIX
        "/qml"
    BASE
        "qml"
    FILES
        ${qml_resource_files}
)
set(qmake_immediate_resource_files
    "qtquickcontrols2.conf"
)

qt6_add_resources(automotive "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

install(TARGETS automotive
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
