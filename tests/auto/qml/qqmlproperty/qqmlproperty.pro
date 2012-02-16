CONFIG += testcase
TARGET = tst_qqmlproperty
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlproperty.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private v8-private qml-private widgets testlib
