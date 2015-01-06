CONFIG += testcase
TEMPLATE = app
TARGET = qmltime
QT += qml testlib quick
QT += quick-private
macx:CONFIG -= app_bundle

SOURCES += qmltime.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
