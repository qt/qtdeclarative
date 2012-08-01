CONFIG += testcase
TARGET = tst_qqmlinstruction
SOURCES += tst_qqmlinstruction.cpp
macx:CONFIG -= app_bundle

CONFIG += parallel_test

QT += core-private gui-private v8-private qml-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
