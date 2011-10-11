CONFIG += testcase
TARGET = tst_qsgflipable
macx:CONFIG -= app_bundle

SOURCES += tst_qsgflipable.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private testlib
