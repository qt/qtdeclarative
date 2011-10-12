CONFIG += testcase
TARGET = tst_qdeclarativexmllistmodel
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativexmllistmodel.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private network testlib xmlpatterns
