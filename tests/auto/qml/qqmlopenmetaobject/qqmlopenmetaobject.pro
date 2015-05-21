CONFIG += testcase
TARGET = tst_qqmlopenmetaobject
osx:CONFIG -= app_bundle

SOURCES += tst_qqmlopenmetaobject.cpp

CONFIG += parallel_test
QT += core-private gui-private qml-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
