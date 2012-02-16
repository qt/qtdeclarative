CONFIG += testcase
TARGET = tst_qqmlsqldatabase
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlsqldatabase.cpp

include (../../shared/util.pri)

CONFIG += parallel_test

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private v8-private qml-private quick-private sql testlib
