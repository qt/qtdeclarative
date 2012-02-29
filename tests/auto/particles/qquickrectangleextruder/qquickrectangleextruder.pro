CONFIG += testcase
TARGET = tst_qquickrectangleextruder
SOURCES += tst_qquickrectangleextruder.cpp
macx:CONFIG -= app_bundle

include (../../shared/util.pri)
TESTDATA = data/*

QT += core-private gui-private v8-private qml-private quick-private testlib

