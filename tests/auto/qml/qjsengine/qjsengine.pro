CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qjsengine
QT += v8-private qml widgets testlib gui-private
macx:CONFIG -= app_bundle
SOURCES += tst_qjsengine.cpp

TESTDATA = script/*
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
