CONFIG += testcase
TARGET = tst_qquickspriteimage
SOURCES += tst_qquickspriteimage.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private quick-private network testlib
