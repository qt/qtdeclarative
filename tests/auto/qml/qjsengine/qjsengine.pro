CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qjsengine
QT += v8-private qml widgets testlib
macx:CONFIG -= app_bundle
SOURCES += tst_qjsengine.cpp

TESTDATA = script/*
