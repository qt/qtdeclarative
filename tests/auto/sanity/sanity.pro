TEMPLATE = app
TARGET = tst_sanity

QT += qml testlib core-private qml-private
CONFIG += testcase
osx:CONFIG -= app_bundle

SOURCES += \
    $$PWD/tst_sanity.cpp
