CONFIG += testcase
TARGET = tst_qquickpathview
macx:CONFIG -= app_bundle

SOURCES += tst_qquickpathview.cpp

include (../../shared/util.pri)
include (../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private v8-private qml-private quick-private widgets testlib
