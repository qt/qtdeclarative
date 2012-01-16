TARGET = tst_qquickanchors
CONFIG += testcase
SOURCES += tst_qquickanchors.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private quick-private v8-private testlib
