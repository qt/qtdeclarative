TEMPLATE = app
TARGET = tst_creationtime

QT += qml testlib
CONFIG += testcase
osx:CONFIG -= app_bundle

DEFINES += QQC2_IMPORT_PATH=\\\"$$absolute_path(../../../src/imports)\\\"

SOURCES += \
    tst_creationtime.cpp
