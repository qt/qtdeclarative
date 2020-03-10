CXX_MODULE = qml
TARGET  = windowplugin
TARGETPATH = QtQuick/Window
QML_IMPORT_VERSION = $$QT_VERSION

SOURCES += \
    plugin.cpp

HEADERS += \
    plugin.h

QT += quick-private qml-private

CONFIG += qmltypes install_qmltypes
load(qml_plugin)
