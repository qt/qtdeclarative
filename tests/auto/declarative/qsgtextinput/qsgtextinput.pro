CONFIG += testcase
TARGET = tst_qsgtextinput
macx:CONFIG -= app_bundle

SOURCES += tst_qsgtextinput.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private v8-private declarative-private opengl-private testlib
