CONFIG += testcase
TARGET = tst_parserstress
macx:CONFIG -= app_bundle

SOURCES += tst_parserstress.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"
DEFINES += TESTDATADIR=\\\"$$PWD/tests\\\"

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
