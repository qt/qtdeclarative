CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qjsvalue
macx:CONFIG -= app_bundle
QT += qml widgets testlib
SOURCES  += tst_qjsvalue.cpp
HEADERS  += tst_qjsvalue.h
