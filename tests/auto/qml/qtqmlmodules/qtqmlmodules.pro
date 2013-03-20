CONFIG += testcase
TARGET = tst_qtqmlmodules
SOURCES += tst_qtqmlmodules.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

TESTDATA = data/*

QT += core-private v8-private qml-private testlib gui gui-private
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
