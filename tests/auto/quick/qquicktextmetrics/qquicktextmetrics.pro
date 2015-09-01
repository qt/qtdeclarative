CONFIG += testcase
TARGET = tst_qquicktextmetrics
osx:CONFIG -= app_bundle

SOURCES += tst_qquicktextmetrics.cpp

CONFIG += parallel_test

QT += core gui qml quick-private testlib
