CONFIG += testcase
TARGET = tst_qquickwheelhandler
macos:CONFIG -= app_bundle

SOURCES += tst_qquickwheelhandler.cpp
OTHER_FILES = \
    data/rectWheel.qml \

include (../../../shared/util.pri)
include (../../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private qml-private quick-private testlib
