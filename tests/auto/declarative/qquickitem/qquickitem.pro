CONFIG += testcase
TARGET = tst_qquickitem
SOURCES += tst_qquickitem.cpp

macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test
QT += core-private gui-private v8-private declarative-private widgets testlib
