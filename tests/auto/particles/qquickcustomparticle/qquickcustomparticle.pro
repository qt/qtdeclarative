CONFIG += testcase
TARGET = tst_qquickcustomparticle
SOURCES += tst_qquickcustomparticle.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private v8-private qml-private quick-private opengl-private testlib

