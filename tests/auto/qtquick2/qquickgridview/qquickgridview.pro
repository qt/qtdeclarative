CONFIG += testcase
TARGET = tst_qquickgridview
macx:CONFIG -= app_bundle

SOURCES += tst_qquickgridview.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test
CONFIG += insignificant_test #QTBUG-22807
QT += core-private gui-private v8-private declarative-private quick-private opengl-private testlib widgets
