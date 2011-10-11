CONFIG += testcase
TARGET = tst_qsggravity
SOURCES += tst_qsggravity.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += insignificant_test

QT += core-private gui-private v8-private declarative-private opengl-private testlib

