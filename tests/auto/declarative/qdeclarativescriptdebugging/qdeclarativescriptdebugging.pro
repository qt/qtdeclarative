CONFIG += testcase
TARGET = tst_qdeclarativescriptdebugging
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativescriptdebugging.cpp
INCLUDEPATH += ../shared

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
