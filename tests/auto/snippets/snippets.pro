TEMPLATE = app
TARGET = tst_snippets

QT += quick testlib
CONFIG += testcase
osx:CONFIG -= app_bundle

SOURCES += \
    $$PWD/tst_snippets.cpp

TESTDATA += \
    $$PWD/data/*
