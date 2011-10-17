CONFIG += testcase
TARGET = tst_qsgrectangleextruder
SOURCES += tst_qsgrectangleextruder.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private v8-private declarative-private opengl-private testlib

