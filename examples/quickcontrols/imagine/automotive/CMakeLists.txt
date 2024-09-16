# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(automotive LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(automotiveexample WIN32 MACOSX_BUNDLE
    automotive.cpp
)

qt_add_qml_module(automotiveexample
    URI automotive
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        "qml/FeatureButton.qml"
        "qml/LargeLabel.qml"
        "qml/automotive.qml"

    RESOURCES
        "icons/automotive/44x44/air-con.png"
        "icons/automotive/44x44/command.png"
        "icons/automotive/44x44/message.png"
        "icons/automotive/44x44/music.png"
        "icons/automotive/44x44/seats.png"
        "icons/automotive/44x44/settings.png"
        "icons/automotive/44x44/statistics.png"
        "icons/automotive/44x44/windows.png"
        "icons/automotive/44x44@2/air-con.png"
        "icons/automotive/44x44@2/command.png"
        "icons/automotive/44x44@2/message.png"
        "icons/automotive/44x44@2/music.png"
        "icons/automotive/44x44@2/navigation.png"
        "icons/automotive/44x44@2/seats.png"
        "icons/automotive/44x44@2/settings.png"
        "icons/automotive/44x44@2/statistics.png"
        "icons/automotive/44x44@2/windows.png"
        "icons/automotive/icons.svg"
        "icons/automotive/index.theme"
        "icons/car.png"
        "icons/car@2x.png"
        "icons/warning.png"
        "icons/warning@2x.png"
        "icons/weather.png"
        "icons/weather@2x.png"
        "imagine-assets/applicationwindow-background.png"
        "imagine-assets/applicationwindow-background@2x.png"
        "imagine-assets/button-background-checked-hovered.9.png"
        "imagine-assets/button-background-checked-hovered@2x.9.png"
        "imagine-assets/button-background-checked.9.png"
        "imagine-assets/button-background-checked@2x.9.png"
        "imagine-assets/button-background-hovered.9.png"
        "imagine-assets/button-background-hovered@2x.9.png"
        "imagine-assets/button-background-pressed.9.png"
        "imagine-assets/button-background-pressed@2x.9.png"
        "imagine-assets/button-background.9.png"
        "imagine-assets/button-background@2x.9.png"
        "imagine-assets/dial-background-hovered.png"
        "imagine-assets/dial-background-hovered@2x.png"
        "imagine-assets/dial-background-pressed.png"
        "imagine-assets/dial-background-pressed@2x.png"
        "imagine-assets/dial-background.png"
        "imagine-assets/dial-background@2x.png"
        "imagine-assets/dial-handle-pressed.png"
        "imagine-assets/dial-handle-pressed@2x.png"
        "imagine-assets/dial-handle.png"
        "imagine-assets/dial-handle@2x.png"
        "imagine-assets/frame-background.9.png"
        "imagine-assets/frame-background@2x.9.png"
        "imagine-assets/itemdelegate-background-checked.9.png"
        "imagine-assets/itemdelegate-background-checked@2x.9.png"
        "imagine-assets/itemdelegate-background-hovered.9.png"
        "imagine-assets/itemdelegate-background-hovered@2x.9.png"
        "imagine-assets/itemdelegate-background-pressed.9.png"
        "imagine-assets/itemdelegate-background-pressed@2x.9.png"
        "imagine-assets/itemdelegate-background.9.png"
        "imagine-assets/itemdelegate-background@2x.9.png"
        "imagine-assets/radiobutton-indicator-checked-hovered.png"
        "imagine-assets/radiobutton-indicator-checked-hovered@2x.png"
        "imagine-assets/radiobutton-indicator-checked-pressed.png"
        "imagine-assets/radiobutton-indicator-checked-pressed@2x.png"
        "imagine-assets/radiobutton-indicator-checked.png"
        "imagine-assets/radiobutton-indicator-checked@2x.png"
        "imagine-assets/radiobutton-indicator-hovered.png"
        "imagine-assets/radiobutton-indicator-hovered@2x.png"
        "imagine-assets/radiobutton-indicator-pressed.png"
        "imagine-assets/radiobutton-indicator-pressed@2x.png"
        "imagine-assets/radiobutton-indicator.png"
        "imagine-assets/radiobutton-indicator@2x.png"
        "imagine-assets/scrollindicator-handle.png"
        "imagine-assets/scrollindicator-handle@2x.png"
        "imagine-assets/slider-background-horizontal.9.png"
        "imagine-assets/slider-background-horizontal@2x.9.png"
        "imagine-assets/slider-handle-hovered.png"
        "imagine-assets/slider-handle-hovered@2x.png"
        "imagine-assets/slider-handle-pressed.png"
        "imagine-assets/slider-handle-pressed@2x.png"
        "imagine-assets/slider-handle.png"
        "imagine-assets/slider-handle@2x.png"
        "imagine-assets/slider-progress-horizontal-pressed.9.png"
        "imagine-assets/slider-progress-horizontal-pressed@2x.9.png"
        "imagine-assets/slider-progress-horizontal.9.png"
        "imagine-assets/slider-progress-horizontal@2x.9.png"
        "imagine-assets/switchdelegate-background.9.png"
        "imagine-assets/switchdelegate-background@2x.9.png"
        "imagine-assets/switchdelegate-handle-checked-hovered.png"
        "imagine-assets/switchdelegate-handle-checked-hovered@2x.png"
        "imagine-assets/switchdelegate-handle-checked.png"
        "imagine-assets/switchdelegate-handle-checked@2x.png"
        "imagine-assets/switchdelegate-handle-hovered.png"
        "imagine-assets/switchdelegate-handle-hovered@2x.png"
        "imagine-assets/switchdelegate-handle-pressed.png"
        "imagine-assets/switchdelegate-handle-pressed@2x.png"
        "imagine-assets/switchdelegate-handle.png"
        "imagine-assets/switchdelegate-handle@2x.png"
        "imagine-assets/switchdelegate-indicator-pressed.png"
        "imagine-assets/switchdelegate-indicator-pressed@2x.png"
        "imagine-assets/switchdelegate-indicator.png"
        "imagine-assets/switchdelegate-indicator@2x.png"
        "imagine-assets/toolseparator-separator-vertical.9.png"
        "imagine-assets/toolseparator-separator-vertical@2x.9.png"
        "qtquickcontrols2.conf"
)

target_link_libraries(automotiveexample PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

install(TARGETS automotiveexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET automotiveexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
