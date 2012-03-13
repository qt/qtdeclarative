CONFIG += testcase
TARGET = tst_examples
macx:CONFIG -= app_bundle

SOURCES += tst_examples.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
#temporary
QT += core-private gui-private qml-private quick-private v8-private testlib

cross_compile: DEFINES += QTEST_CROSS_COMPILED
