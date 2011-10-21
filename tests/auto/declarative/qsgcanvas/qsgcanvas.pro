CONFIG += testcase
TARGET = tst_qsgcanvas
SOURCES += tst_qsgcanvas.cpp

macx:CONFIG -= app_bundle

CONFIG += parallel_test
QT += core-private gui-private declarative-private testlib
