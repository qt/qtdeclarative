CONFIG += testcase
TARGET = tst_qmlimportscanner
macos:CONFIG -= app_bundle

SOURCES += tst_qmlimportscanner.cpp

include (../../shared/util.pri)

TESTDATA = data/*

QT += testlib
