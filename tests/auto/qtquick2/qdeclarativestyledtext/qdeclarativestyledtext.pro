CONFIG += testcase
TARGET = tst_qdeclarativestyledtext
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativestyledtext.cpp

CONFIG += parallel_test
QT += core-private gui-private declarative-private quick-private network testlib
