CONFIG += testcase
TARGET = tst_qmlmin
QT += qml testlib
macx:CONFIG -= app_bundle

SOURCES += tst_qmlmin.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
