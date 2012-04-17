CONFIG += testcase
TARGET = tst_qqmlglobal
SOURCES += tst_qqmlglobal.cpp
macx:CONFIG -= app_bundle

CONFIG += parallel_test
QT += qml-private testlib
