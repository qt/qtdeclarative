CXX_MODULE = qml
TARGET  = windowplugin
TARGETPATH = QtQuick/Window
IMPORT_VERSION = 2.15

SOURCES += \
    plugin.cpp

HEADERS += \
    plugin.h

QT += quick-private qml-private

CONFIG += qmltypes install_qmltypes
load(qml_plugin)
