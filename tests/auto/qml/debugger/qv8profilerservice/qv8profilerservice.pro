CONFIG += testcase
TARGET = tst_qv8profilerservice
macx:CONFIG -= app_bundle

SOURCES += tst_qv8profilerservice.cpp

INCLUDEPATH += ../shared
include(../../../shared/util.pri)
include(../shared/debugutil.pri)

TESTDATA = data/*

CONFIG += parallel_test

QT += qml testlib

