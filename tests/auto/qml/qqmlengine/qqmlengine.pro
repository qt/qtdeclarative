CONFIG += testcase
TARGET = tst_qqmlengine
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlengine.cpp 

CONFIG += parallel_test

QT += core-private gui-private qml-private network testlib
