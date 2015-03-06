TARGET = QtQuickControls
MODULE = quickcontrols
CONFIG += internal_module

QT += quick
QT += core-private gui-private qml-private quick-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

QMAKE_DOCS = $$PWD/doc/qtquickcontrols2.qdocconf

HEADERS += \
    $$PWD/qtquickcontrolsglobal_p.h

include(controls.pri)
load(qt_module)
