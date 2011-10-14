CONFIG += testcase
TARGET = tst_qquickitem
macx:CONFIG -= app_bundle

SOURCES += tst_qquickitem.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private opengl-private testlib
