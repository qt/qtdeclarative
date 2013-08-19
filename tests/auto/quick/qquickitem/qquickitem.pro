CONFIG += testcase
TARGET = tst_qquickitem
SOURCES += tst_qquickitem.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

TESTDATA = data/*

QT += core-private gui-private  qml-private quick-private testlib

win32:CONFIG += insignificant_test # QTBUG-32664

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
