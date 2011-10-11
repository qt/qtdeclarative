CONFIG += testcase
TARGET = tst_qsgmaskextruder
SOURCES += tst_qsgmaskextruder.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += insignificant_test

QT += core-private gui-private v8-private declarative-private opengl-private testlib

