CONFIG += testcase
TARGET = tst_qquickpathitem
macx:CONFIG -= app_bundle

SOURCES += tst_qquickpathitem.cpp

include (../../shared/util.pri)
include (../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private  qml-private quick-private testlib
qtHaveModule(widgets): QT += widgets
