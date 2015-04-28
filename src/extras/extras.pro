TARGET = QtQuickExtras
MODULE = quickextras2
CONFIG += internal_module

QT += quick
QT += core-private gui-private qml-private quick-private quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

QMAKE_DOCS = $$PWD/doc/qtquickextras2.qdocconf

HEADERS += \
    $$PWD/qtquickextrasglobal_p.h

include(extras.pri)
load(qt_module)
