CONFIG += testcase
TARGET = tst_qqmllistreference
macx:CONFIG -= app_bundle

SOURCES += tst_qqmllistreference.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private qml-private testlib
