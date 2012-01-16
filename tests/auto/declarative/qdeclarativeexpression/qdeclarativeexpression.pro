CONFIG += testcase
TARGET = tst_qdeclarativeexpression
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeexpression.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
