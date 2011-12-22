CONFIG += testcase
TARGET = tst_qdeclarativebinding
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativebinding.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private quick-private testlib
