CONFIG += testcase
TARGET = tst_qdeclarativebinding
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativebinding.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
