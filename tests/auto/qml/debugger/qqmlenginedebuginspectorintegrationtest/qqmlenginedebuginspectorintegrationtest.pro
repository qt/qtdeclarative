CONFIG += testcase
TARGET = tst_qqmlenginedebuginspectorintegration

QT += testlib gui-private core-private
osx:CONFIG -= app_bundle

SOURCES += tst_qqmlenginedebuginspectorintegration.cpp

include(../shared/debugutil.pri)

TESTDATA = data/*
