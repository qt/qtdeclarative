CONFIG += testcase
TARGET = tst_qquickitemparticle
SOURCES += tst_qquickitemparticle.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += insignificant_test    #temporary

QT += core-private gui-private v8-private declarative-private opengl-private testlib

