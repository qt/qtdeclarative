TEMPLATE = lib
CONFIG += plugin
SOURCES = optionalPlugin.cpp
QT = core qml
DESTDIR = ../imports/QtQuick/Shapes

IMPORT_FILES = \
        qmldir

include (../../../shared/imports.pri)
