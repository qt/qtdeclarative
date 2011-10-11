CONFIG += testcase
TARGET = tst_qdeclarativeconnection
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeconnection.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private opengl-private testlib
