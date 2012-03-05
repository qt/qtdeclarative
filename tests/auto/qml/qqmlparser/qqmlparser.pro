CONFIG += testcase
TARGET = tst_qqmlparser
QT += qmldevtools-private testlib
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlparser.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
