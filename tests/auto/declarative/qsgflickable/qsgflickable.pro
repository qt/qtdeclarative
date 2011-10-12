CONFIG += testcase
TARGET = tst_qsgflickable
macx:CONFIG -= app_bundle

SOURCES += tst_qsgflickable.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test
#temporary
CONFIG += insignificant_test
QT += core-private gui-private v8-private declarative-private testlib
