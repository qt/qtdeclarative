CONFIG += testcase
TARGET = tst_qqmlcomponent
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlcomponent.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private qml-private network testlib
