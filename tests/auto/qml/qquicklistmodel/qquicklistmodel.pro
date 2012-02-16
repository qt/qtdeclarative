CONFIG += testcase
TARGET = tst_qquicklistmodel
macx:CONFIG -= app_bundle

SOURCES += tst_qquicklistmodel.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private v8-private qml-private quick-private opengl-private testlib
