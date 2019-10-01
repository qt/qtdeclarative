CONFIG += testcase
TARGET = tst_qquickboundaryrule
macx:CONFIG -= app_bundle

SOURCES += tst_qquickboundaryrule.cpp

include (../../shared/util.pri)
include (../shared/util.pri)

TESTDATA = data/*

QT += quick-private qml testlib
