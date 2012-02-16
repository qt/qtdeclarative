CONFIG += testcase
TARGET = tst_qqmlerror
SOURCES += tst_qqmlerror.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

CONFIG += parallel_test

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private qml-private testlib
