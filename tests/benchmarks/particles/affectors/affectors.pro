CONFIG += testcase
TARGET = tst_affectors
SOURCES += tst_affectors.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private v8-private qml-private quick-private opengl-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
