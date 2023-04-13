# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

QT += quick quickcontrols2

CONFIG += qmltypes

QML_IMPORT_PATH = $$pwd/.
QML_IMPORT_NAME = FileSystemModule
QML_IMPORT_MAJOR_VERSION = 1

TARGET = filesystemexplorer
TEMPLATE = app

SOURCES += \
    main.cpp \
    filesystemmodel.cpp \

HEADERS += \
    filesystemmodel.h

qml_resources.files = \
    qmldir \
    Main.qml \
    qml/Icon.qml \
    qml/About.qml \
    qml/Colors.qml \
    qml/MyMenu.qml \
    qml/Sidebar.qml \
    qml/MyMenuBar.qml \
    qml/ResizeButton.qml \
    qml/FileSystemView.qml \
    qml/WindowDragHandler.qml \

qml_resources.prefix = /qt/qml/FileSystemModule

theme_resources.files = \
    icons/folder_closed.svg \
    icons/folder_open.svg \
    icons/generic_file.svg \
    icons/globe.svg \
    icons/info_sign.svg \
    icons/light_bulb.svg \
    icons/read.svg \
    icons/resize.svg \
    icons/qt_logo.svg \

theme_resources.prefix = /qt/qml/FileSystemModule

RESOURCES += qml_resources theme_resources

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols/filesystemexplorer
INSTALLS += target
