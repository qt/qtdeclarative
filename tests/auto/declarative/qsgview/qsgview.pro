CONFIG += testcase
TARGET = tst_qsgview
macx:CONFIG -= app_bundle

SOURCES += tst_qsgview.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private declarative-private testlib
