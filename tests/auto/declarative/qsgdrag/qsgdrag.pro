TARGET = tst_qsgdrag
CONFIG += testcase
macx:CONFIG -= app_bundle

SOURCES += tst_qsgdrag.cpp

CONFIG += parallel_test

QT += core-private gui-private declarative-private network testlib
