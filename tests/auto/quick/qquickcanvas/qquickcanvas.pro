CONFIG += testcase
TARGET = tst_qquickcanvas
SOURCES += tst_qquickcanvas.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

CONFIG += parallel_test
QT += core-private gui-private qml-private quick-private v8-private testlib

TESTDATA = data/*

OTHER_FILES += \
    data/AnimationsWhileHidden.qml \
    data/Headless.qml

