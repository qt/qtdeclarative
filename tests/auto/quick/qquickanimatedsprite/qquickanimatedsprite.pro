CONFIG += testcase
TARGET = tst_qquickanimatedsprite
SOURCES += tst_qquickanimatedsprite.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private qml-private quick-private network testlib
