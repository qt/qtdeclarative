TARGET = QtQuickControls
MODULE = quickcontrols

QT += quick
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

HEADERS += \
    $$PWD/qtquickcontrolsglobal.h
    $$PWD/qtquickcontrolsglobal_p.h

include(controls.pri)
load(qt_module)
