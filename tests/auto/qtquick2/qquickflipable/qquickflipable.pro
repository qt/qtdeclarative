CONFIG += testcase
TARGET = tst_qquickflipable
macx:CONFIG -= app_bundle

SOURCES += tst_qquickflipable.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private quick-private testlib
