CONFIG += testcase
TARGET = tst_qquickpath
macx:CONFIG -= app_bundle

SOURCES += tst_qquickpath.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private v8-private qml-private quick-private testlib
