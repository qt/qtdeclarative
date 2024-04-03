CONFIG += testcase
TARGET = tst_qv4estable
include (../../shared/util.pri)

macos:CONFIG -= app_bundle

TESTDATA = data/*

SOURCES += tst_qv4estable.cpp

QT += qml qml-private testlib

