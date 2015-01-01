CONFIG += testcase
TARGET = tst_qquickshortcut

SOURCES += tst_qquickshortcut.cpp

include (../../shared/util.pri)

TESTDATA = data/*

QT += core gui qml quick testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
