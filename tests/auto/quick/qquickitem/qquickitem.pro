CONFIG += testcase
TARGET = tst_qquickitem
SOURCES += tst_qquickitem.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

TESTDATA = data/*

CONFIG += parallel_test
QT += core-private gui-private v8-private qml-private quick-private widgets testlib
