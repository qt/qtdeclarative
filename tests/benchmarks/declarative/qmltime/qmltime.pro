CONFIG += testcase
TEMPLATE = app
TARGET = qmltime
QT += declarative widgets testlib
macx:CONFIG -= app_bundle

SOURCES += qmltime.cpp 

