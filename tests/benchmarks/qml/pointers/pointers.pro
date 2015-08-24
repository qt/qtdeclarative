CONFIG += testcase
QT += core-private qml-private testlib
TEMPLATE = app
TARGET = tst_pointers
macx:CONFIG -= app_bundle

SOURCES += tst_pointers.cpp
