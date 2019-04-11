CONFIG += testcase
TARGET = tst_qquickheaderview
SOURCES += tst_qquickheaderview.cpp

macos:CONFIG -= app_bundle

QT += core-private gui-private qml-private quick-private testlib quickcontrols2 \
     quickcontrols2-private quicktemplates2-private quicktemplates2

include (../shared/util.pri)

TESTDATA = data/*

OTHER_FILES += \
    data/*.qml
