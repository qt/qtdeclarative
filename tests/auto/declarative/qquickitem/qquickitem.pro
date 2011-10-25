CONFIG += testcase
TARGET = tst_qquickitem
SOURCES += tst_qquickitem.cpp

macx:CONFIG -= app_bundle

CONFIG += parallel_test
QT += core-private gui-private declarative-private widgets testlib
