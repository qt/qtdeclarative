CONFIG += testcase
TARGET = tst_qqmlpropertycache
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlpropertycache.cpp

CONFIG += parallel_test
QT += core-private gui-private qml-private testlib v8-private
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
