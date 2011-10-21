CONFIG += testcase
TARGET = tst_qsgfocusscope
SOURCES += tst_qsgfocusscope.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private declarative-private testlib
