CONFIG += testcase
TARGET = tst_qquickview_extra
macx:CONFIG -= app_bundle

SOURCES += tst_qquickview_extra.cpp

include (../../shared/util.pri)
include (../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private qml-private quick-private testlib
