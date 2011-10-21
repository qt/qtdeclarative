CONFIG += testcase
TARGET = tst_qsgpathview
macx:CONFIG -= app_bundle

SOURCES += tst_qsgpathview.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test
QT += core-private gui-private v8-private declarative-private widgets testlib
