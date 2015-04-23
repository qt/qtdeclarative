TARGET  = qtquickcalendar2plugin
TARGETPATH = QtQuick/Calendar.2
IMPORT_VERSION = 2.0

QT += qml quick
QT += core-private gui-private qml-private quick-private quickcontrols2-private quickcalendar2-private

OTHER_FILES += \
    qmldir

QML_FILES = \
    CalendarDelegate.qml \
    CalendarView.qml \
    DayOfWeekRow.qml \
    WeekNumberColumn.qml

SOURCES += \
    $$PWD/qtquickcalendar2plugin.cpp

CONFIG += no_cxx_module
load(qml_plugin)
