CONFIG += testcase
TARGET = tst_qsgflipable
macx:CONFIG -= app_bundle

SOURCES += tst_qsgflipable.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private testlib
