CONFIG += testcase
TARGET = tst_qdeclarativeinstruction
SOURCES += tst_qdeclarativeinstruction.cpp
macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private testlib
