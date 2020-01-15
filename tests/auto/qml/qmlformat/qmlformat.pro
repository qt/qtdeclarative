CONFIG += testcase
TARGET = tst_qmlformat
macos:CONFIG -= app_bundle

SOURCES += tst_qmlformat.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

include (../../shared/util.pri)

TESTDATA = data/*

QT += testlib
