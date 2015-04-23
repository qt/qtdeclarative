TARGET = QtQuickCalendar
MODULE = quickcalendar
CONFIG += internal_module

QT += quick
QT += core-private gui-private qml-private quick-private quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

HEADERS += \
    $$PWD/qtquickcalendarglobal_p.h

include(calendar.pri)
load(qt_module)
