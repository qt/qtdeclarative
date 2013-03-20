CONFIG += testcase
TARGET = tst_qv8profilerservice
macx:CONFIG -= app_bundle

SOURCES += tst_qv8profilerservice.cpp

INCLUDEPATH += ../shared
include(../../../shared/util.pri)
include(../shared/debugutil.pri)

TESTDATA = data/*

QT += qml testlib gui-private

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
CONFIG+=insignificant_test
