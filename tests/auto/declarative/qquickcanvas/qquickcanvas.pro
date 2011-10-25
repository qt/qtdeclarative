CONFIG += testcase
TARGET = tst_qquickcanvas
SOURCES += tst_qquickcanvas.cpp

macx:CONFIG -= app_bundle

CONFIG += parallel_test
QT += core-private gui-private declarative-private testlib
