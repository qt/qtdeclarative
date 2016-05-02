CONFIG += testcase
TARGET = tst_qquickrendererinfo
SOURCES += tst_qquickrendererinfo.cpp

TESTDATA = data/*
include(../../shared/util.pri)

osx:CONFIG -= app_bundle

QT += quick testlib

OTHER_FILES += \
    data/basic.qml

