CONFIG += testcase
TARGET = tst_qv4assembler

include (../../shared/util.pri)

macos:CONFIG -= app_bundle

TESTDATA = data/*

SOURCES += tst_qv4assembler.cpp

QT += qml-private testlib
