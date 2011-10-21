TARGET = tst_qsgdroparea
CONFIG += testcase
macx:CONFIG -= app_bundle

SOURCES += tst_qsgdroparea.cpp

CONFIG += parallel_test

QT += core-private gui-private declarative-private network testlib
