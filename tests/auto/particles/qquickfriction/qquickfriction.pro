CONFIG += testcase
TARGET = tst_qquickfriction
SOURCES += tst_qquickfriction.cpp
macx:CONFIG -= app_bundle

include (../../shared/util.pri)
TESTDATA = data/*

QT += core-private gui-private v8-private qml-private quick-private opengl-private testlib

