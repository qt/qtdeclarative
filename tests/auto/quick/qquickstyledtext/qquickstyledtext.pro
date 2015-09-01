CONFIG += testcase
TARGET = tst_qquickstyledtext
macx:CONFIG -= app_bundle

SOURCES += tst_qquickstyledtext.cpp

CONFIG += parallel_test
QT += core-private gui-private qml-private quick-private network testlib
