CONFIG += testcase
TARGET = tst_qdeclarativespringanimation
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativespringanimation.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private testlib
