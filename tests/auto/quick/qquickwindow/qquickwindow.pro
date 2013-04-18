CONFIG += testcase
TARGET = tst_qquickwindow
SOURCES += tst_qquickwindow.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

QT += core-private gui-private qml-private quick-private v8-private testlib

TESTDATA = data/*

OTHER_FILES += \
    data/active.qml \
    data/AnimationsWhileHidden.qml \
    data/Headless.qml \
    data/showHideAnimate.qml

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
