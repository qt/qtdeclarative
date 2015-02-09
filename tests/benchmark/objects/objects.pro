TEMPLATE = app
TARGET = tst_objects

QT += quick testlib core-private
CONFIG += testcase
osx:CONFIG -= app_bundle

SOURCES += \
    tst_objects.cpp
