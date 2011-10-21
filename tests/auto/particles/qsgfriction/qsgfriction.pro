CONFIG += testcase
TARGET = tst_qsgfriction
SOURCES += tst_qsgfriction.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private v8-private declarative-private opengl-private testlib

