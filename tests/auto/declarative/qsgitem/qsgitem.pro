CONFIG += testcase
TARGET = tst_qsgitem
SOURCES += tst_qsgitem.cpp

macx:CONFIG -= app_bundle

CONFIG += parallel_test
QT += core-private gui-private declarative-private widgets testlib
