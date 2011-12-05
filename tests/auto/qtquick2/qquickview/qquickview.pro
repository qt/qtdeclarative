CONFIG += testcase
TARGET = tst_qquickview
macx:CONFIG -= app_bundle

SOURCES += tst_qquickview.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private declarative-private quick-private testlib
