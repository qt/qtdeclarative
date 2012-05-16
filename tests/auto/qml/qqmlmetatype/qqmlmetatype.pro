CONFIG += testcase
TARGET = tst_qqmlmetatype
SOURCES += tst_qqmlmetatype.cpp
macx:CONFIG -= app_bundle

CONFIG += parallel_test
QT += core-private gui-private qml-private testlib v8-private
