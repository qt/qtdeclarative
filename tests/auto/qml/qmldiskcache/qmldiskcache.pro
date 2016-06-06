CONFIG += testcase
TARGET = tst_qmldiskcache
osx:CONFIG -= app_bundle

SOURCES += tst_qmldiskcache.cpp

QT += core-private qml-private testlib
