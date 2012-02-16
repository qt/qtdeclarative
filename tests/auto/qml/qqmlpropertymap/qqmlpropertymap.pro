CONFIG += testcase
TARGET = tst_qqmlpropertymap
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlpropertymap.cpp

CONFIG += parallel_test

QT += core-private gui-private qml-private quick-private testlib
