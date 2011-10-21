CONFIG += testcase
TARGET = tst_qmlplugindump
QT += testlib
macx:CONFIG -= app_bundle
CONFIG += parallel_test

SOURCES += tst_qmlplugindump.cpp
