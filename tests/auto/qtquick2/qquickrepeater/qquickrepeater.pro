CONFIG += testcase
TARGET = tst_qquickrepeater
macx:CONFIG -= app_bundle

SOURCES += tst_qquickrepeater.cpp

testFiles.files = data
testFiles.path = .
DEPLOYMENT += testFiles

CONFIG += parallel_test
QT += core-private gui-private declarative-private quick-private testlib
