CONFIG += testcase
TARGET = tst_qquickscreen
SOURCES += tst_qquickscreen.cpp

macx:CONFIG -= app_bundle

CONFIG += parallel_test
QT += core-private gui-private declarative-private testlib
