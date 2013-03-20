CONFIG += testcase
TARGET = tst_dialogs
SOURCES += tst_dialogs.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

CONFIG += parallel_test
QT += core-private gui-private qml-private quick-private v8-private testlib

TESTDATA = data/*

OTHER_FILES += \
    data/FileDialog.qml \

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
