CONFIG += testcase
TARGET = tst_qsgvisualdatamodel
macx:CONFIG -= app_bundle

SOURCES += tst_qsgvisualdatamodel.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private widgets testlib
