TEMPLATE = app
TARGET = tst_creation

QT += qml testlib
CONFIG += testcase
osx:CONFIG -= app_bundle

SOURCES += \
    tst_creation.cpp
