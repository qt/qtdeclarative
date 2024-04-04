# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

set_source_files_properties(CheckBox.qml TabSet.qml TextField.qml
    PROPERTIES
        QT_QML_SOURCE_VERSIONS 2.1
)

set_source_files_properties(Images.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE true
        QT_QML_SOURCE_VERSIONS 2.2
)

qt_add_qml_module(${PROJECT_NAME}_shared
    URI shared
    VERSION 2.2
    PLUGIN_TARGET ${PROJECT_NAME}_shared
    SOURCES
        "shared.h"
    QML_FILES
        "Button.qml"
        "CheckBox.qml"
        "Label.qml"
        "LauncherList.qml"
        "SimpleLauncherDelegate.qml"
        "Slider.qml"
        "TabSet.qml"
        "TextField.qml"
        "Images.qml"
    RESOURCES
        "images/back.png"
        "images/checkmark.png"
        "images/next.png"
        "images/qt-logo.png"
        "images/slider_handle.png"
        "images/tab.png"
)
