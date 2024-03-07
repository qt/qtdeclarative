# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(gallery_controls2 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

qt_add_executable(galleryexample WIN32 MACOSX_BUNDLE
    gallery.cpp
)

qt_add_qml_module(galleryexample
    URI gallery
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        "+Material/ToolBar.qml"
        "ToolBar.qml"
        "gallery.qml"
        "pages/BusyIndicatorPage.qml"
        "pages/ButtonPage.qml"
        "pages/CheckBoxPage.qml"
        "pages/ComboBoxPage.qml"
        "pages/DelayButtonPage.qml"
        "pages/DelegatePage.qml"
        "pages/DialPage.qml"
        "pages/DialogPage.qml"
        "pages/FramePage.qml"
        "pages/GroupBoxPage.qml"
        "pages/PageIndicatorPage.qml"
        "pages/ProgressBarPage.qml"
        "pages/RadioButtonPage.qml"
        "pages/RangeSliderPage.qml"
        "pages/ScrollBarPage.qml"
        "pages/ScrollIndicatorPage.qml"
        "pages/ScrollablePage.qml"
        "pages/SliderPage.qml"
        "pages/SpinBoxPage.qml"
        "pages/StackViewPage.qml"
        "pages/SwipeViewPage.qml"
        "pages/SwitchPage.qml"
        "pages/TabBarPage.qml"
        "pages/TextAreaPage.qml"
        "pages/TextFieldPage.qml"
        "pages/ToolTipPage.qml"
        "pages/TumblerPage.qml"
    RESOURCES
        "icons/gallery/20x20/back.png"
        "icons/gallery/20x20/drawer.png"
        "icons/gallery/20x20/menu.png"
        "icons/gallery/20x20@2/back.png"
        "icons/gallery/20x20@2/drawer.png"
        "icons/gallery/20x20@2/menu.png"
        "icons/gallery/20x20@3/back.png"
        "icons/gallery/20x20@3/drawer.png"
        "icons/gallery/20x20@3/menu.png"
        "icons/gallery/20x20@4/back.png"
        "icons/gallery/20x20@4/drawer.png"
        "icons/gallery/20x20@4/menu.png"
        "icons/gallery/index.theme"
        "images/arrow.png"
        "images/arrow@2x.png"
        "images/arrow@3x.png"
        "images/arrow@4x.png"
        "images/arrows.png"
        "images/arrows@2x.png"
        "images/arrows@3x.png"
        "images/arrows@4x.png"
        "images/qt-logo.png"
        "images/qt-logo@2x.png"
        "images/qt-logo@3x.png"
        "images/qt-logo@4x.png"
        "qmldir"
        "qtquickcontrols2.conf"
)

target_link_libraries(galleryexample PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

if(UNIX AND NOT APPLE AND CMAKE_CROSSCOMPILING)
    find_package(Qt6 REQUIRED COMPONENTS QuickTemplates2)

    # Work around QTBUG-86533
    target_link_libraries(galleryexample PRIVATE Qt6::QuickTemplates2)
endif()

install(TARGETS galleryexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET galleryexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
