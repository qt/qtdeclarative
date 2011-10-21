CONFIG += testcase
TARGET = tst_qsggridview
macx:CONFIG -= app_bundle

SOURCES += tst_qsggridview.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test
#temporary
CONFIG += insignificant_test
QT += core-private gui-private v8-private declarative-private opengl-private testlib
