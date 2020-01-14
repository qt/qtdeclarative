CXX_MODULE = qml
TARGET  = windowplugin
TARGETPATH = QtQuick/Window.2
IMPORT_VERSION = 2.$$QT_MINOR_VERSION

SOURCES += \
    plugin.cpp

HEADERS += \
    plugin.h

QT += quick-private qml-private

CONFIG += qmltypes install_qmltypes
load(qml_plugin)
