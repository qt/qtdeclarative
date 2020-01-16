CONFIG += testcase
TARGET = tst_qmlformat
macos:CONFIG -= app_bundle

SOURCES += tst_qmlformat.cpp

include (../../shared/util.pri)

TESTDATA = data/*

QT += testlib
