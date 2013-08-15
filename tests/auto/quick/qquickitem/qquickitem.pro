CONFIG += testcase
TARGET = tst_qquickitem
SOURCES += tst_qquickitem.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

TESTDATA = data/*

win32:CONFIG += insignificant_test # QTBUG-32664

QT += core-private gui-private v8-private qml-private quick-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
