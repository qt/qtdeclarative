CONFIG += testcase
TARGET = tst_qquickpathview
macx:CONFIG -= app_bundle

SOURCES += tst_qquickpathview.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test
QT += core-private gui-private v8-private declarative-private quick-private widgets testlib
