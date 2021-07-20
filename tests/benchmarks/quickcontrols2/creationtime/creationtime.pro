TEMPLATE = app
TARGET = tst_creationtime

QT += qml testlib quickcontrols2
CONFIG += testcase
macos:CONFIG -= app_bundle

include(../../../auto/shared/util.pri)

SOURCES += \
    tst_creationtime.cpp
