CONFIG += testcase
TARGET = tst_qdeclarativeerror
SOURCES += tst_qdeclarativeerror.cpp
macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
