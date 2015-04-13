TEMPLATE = app
TARGET = tst_declarative

QT += qml testlib core-private qml-private
CONFIG += testcase
osx:CONFIG -= app_bundle

SOURCES += \
    $$PWD/tst_declarative.cpp
