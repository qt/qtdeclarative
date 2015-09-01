CONFIG += testcase
TARGET = tst_qqmlstatemachine
osx:CONFIG -= app_bundle

SOURCES += tst_qqmlstatemachine.cpp

include (../../shared/util.pri)

CONFIG += parallel_test
QT += core-private gui-private qml-private quick-private gui testlib
