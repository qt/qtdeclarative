CONFIG += testcase
TARGET = tst_qdeclarativeparser
QT += qmldevtools-private testlib
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeparser.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
