CONFIG += testcase
TARGET = tst_qqmltablemodel

SOURCES += tst_qqmltablemodel.cpp

include (../../shared/util.pri)

TESTDATA = data/*

QT += core gui qml-private qml quick-private quick testlib
