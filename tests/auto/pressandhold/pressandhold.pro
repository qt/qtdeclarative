TEMPLATE = app
TARGET = tst_pressandhold

QT += quick testlib quickcontrols2
CONFIG += testcase
macos:CONFIG -= app_bundle

SOURCES += \
    $$PWD/tst_pressandhold.cpp
