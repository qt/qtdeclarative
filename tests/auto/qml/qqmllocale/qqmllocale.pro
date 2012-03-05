CONFIG += testcase
TARGET = tst_qqmllocale
macx:CONFIG -= app_bundle

SOURCES += tst_qqmllocale.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += qml testlib
