CONFIG += testcase
TARGET = tst_qdeclarativeinfo
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeinfo.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
QT += core-private gui-private declarative-private widgets testlib
