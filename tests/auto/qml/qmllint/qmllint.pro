CONFIG += testcase
TARGET = tst_qmllint
macos:CONFIG -= app_bundle

SOURCES += tst_qmllint.cpp

include (../../shared/util.pri)

TESTDATA = data/*

QT += testlib
