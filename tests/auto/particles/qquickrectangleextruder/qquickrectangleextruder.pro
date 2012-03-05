CONFIG += testcase
TARGET = tst_qquickrectangleextruder
SOURCES += tst_qquickrectangleextruder.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private v8-private qml-private quick-private opengl-private testlib

