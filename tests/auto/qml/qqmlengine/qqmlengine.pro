CONFIG += testcase
TARGET = tst_qqmlengine
macx:CONFIG -= app_bundle

include (../../shared/util.pri)

SOURCES += tst_qqmlengine.cpp 

CONFIG += parallel_test

QT += core-private gui-private qml-private v8-private network testlib
