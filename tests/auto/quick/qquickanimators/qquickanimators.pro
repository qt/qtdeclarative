CONFIG += testcase
TARGET = tst_qquickanimators
SOURCES += tst_qquickanimators.cpp

include (../../shared/util.pri)
include (../shared/util.pri)

macos:CONFIG -= app_bundle

TESTDATA = data/*

QT += core-private gui-private  qml-private quick-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

OTHER_FILES += \
    data/positionerWithAnimator.qml \
    data/windowWithAnimator.qml \
    data/animatorImplicitFrom.qml
