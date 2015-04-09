TEMPLATE = app
TARGET = tst_creationtime

QT += qml testlib
CONFIG += testcase
osx:CONFIG -= app_bundle

SOURCES += \
    tst_creationtime.cpp
