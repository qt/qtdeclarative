CONFIG += testcase
TARGET = tst_qquickage
SOURCES += tst_qquickage.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += insignificant_test

QT += core-private gui-private v8-private declarative-private quick-private opengl-private testlib

