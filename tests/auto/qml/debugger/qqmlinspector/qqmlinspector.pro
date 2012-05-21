CONFIG += testcase
TARGET = tst_qqmlinspector

QT += qml testlib
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlinspector.cpp

INCLUDEPATH += ../shared
include(../../../shared/util.pri)
include(../shared/debugutil.pri)

TESTDATA = data/*
