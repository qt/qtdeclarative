CONFIG += testcase
TARGET = tst_qquicktextinput
macx:CONFIG -= app_bundle

SOURCES += tst_qquicktextinput.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private v8-private qml-private quick-private opengl-private testlib
