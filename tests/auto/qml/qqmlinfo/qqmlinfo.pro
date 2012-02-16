CONFIG += testcase
TARGET = tst_qqmlinfo
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlinfo.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test
QT += core-private gui-private qml-private widgets testlib
