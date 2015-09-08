TARGET  = qtquickextras2plugin
TARGETPATH = QtQuick/Extras.2
IMPORT_VERSION = 2.0

QT += qml quick
QT += core-private gui-private qml-private quick-private quickcontrols2-private

QMAKE_DOCS = $$PWD/doc/qtquickextras2.qdocconf

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

INCLUDEPATH += $$PWD

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtquickextras2plugin.cpp

include(extras.pri)

CONFIG += no_cxx_module
load(qml_plugin)
