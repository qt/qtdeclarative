CONFIG += testcase
TARGET = tst_qquickpincharea
macx:CONFIG -= app_bundle

SOURCES += tst_qquickpincharea.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private quick-private testlib
