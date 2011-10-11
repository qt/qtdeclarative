CONFIG += testcase
TARGET = tst_qdeclarativesqldatabase
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativesqldatabase.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private sql testlib
