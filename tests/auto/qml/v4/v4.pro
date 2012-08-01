CONFIG += testcase
TARGET = tst_qqmlv4
macx:CONFIG -= app_bundle

SOURCES += tst_v4.cpp \
           testtypes.cpp
HEADERS += testtypes.h

include (../../shared/util.pri)

TESTDATA = data/*

CONFIG += parallel_test

QT += core-private gui-private v8-private qml-private network testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
