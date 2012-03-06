CONFIG += testcase
TARGET = tst_qquickspriteimage
SOURCES += tst_qquickspriteimage.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

TESTDATA = data/*

CONFIG += parallel_test

QT += core-private gui-private qml-private quick-private network testlib
