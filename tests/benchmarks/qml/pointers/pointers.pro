CONFIG += testcase
QT += qml testlib
TEMPLATE = app
TARGET = tst_pointers
macx:CONFIG -= app_bundle

SOURCES += tst_pointers.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
