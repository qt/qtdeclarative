CONFIG += testcase
TARGET = tst_qqmlimageprovider
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlimageprovider.cpp

CONFIG += parallel_test

QT += core-private gui-private qml-private quick-private network testlib
