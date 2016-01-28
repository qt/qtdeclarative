CONFIG += testcase
TARGET = tst_popup
SOURCES += tst_popup.cpp

osx:CONFIG -= app_bundle

QT += core-private gui-private qml-private quick-private testlib labstemplates-private

include (../shared/util.pri)

TESTDATA = data/*

OTHER_FILES += \
    data/*
