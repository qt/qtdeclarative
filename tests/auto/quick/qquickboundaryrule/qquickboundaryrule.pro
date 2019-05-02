CONFIG += testcase
TARGET = tst_qquickboundaryrule
macx:CONFIG -= app_bundle

SOURCES += tst_qquickboundaryrule.cpp

include (../../shared/util.pri)
include (../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private  qml-private quick-private testlib
