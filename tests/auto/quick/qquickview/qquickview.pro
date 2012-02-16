CONFIG += testcase
TARGET = tst_qquickview
macx:CONFIG -= app_bundle

SOURCES += tst_qquickview.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private qml-private quick-private testlib
