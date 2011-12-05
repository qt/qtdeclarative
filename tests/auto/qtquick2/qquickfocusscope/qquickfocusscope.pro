CONFIG += testcase
TARGET = tst_qquickfocusscope
SOURCES += tst_qquickfocusscope.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private declarative-private quick-private testlib
