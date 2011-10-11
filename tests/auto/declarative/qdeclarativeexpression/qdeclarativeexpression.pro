CONFIG += testcase
TARGET = tst_qdeclarativeexpression
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeexpression.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
