CONFIG += testcase
TEMPLATE = app
TARGET = qmltime
QT += qml widgets testlib
macx:CONFIG -= app_bundle

SOURCES += qmltime.cpp 

