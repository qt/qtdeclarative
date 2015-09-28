TARGET  = qtquickcalendar2plugin
TARGETPATH = QtQuick/Calendar.2
IMPORT_VERSION = 2.0

QT += qml quick
QT += core-private gui-private qml-private quick-private quicktemplates-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

INCLUDEPATH += $$PWD

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtquickcalendar2plugin.cpp

include(calendar.pri)

CONFIG += no_cxx_module
load(qml_plugin)
