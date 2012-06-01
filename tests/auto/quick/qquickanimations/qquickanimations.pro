CONFIG += testcase
TARGET = tst_qquickanimations
SOURCES += tst_qquickanimations.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

TESTDATA = data/*

CONFIG += parallel_test

QT += core-private gui-private v8-private qml-private quick-private testlib

# QTBUG-23385 - color mixing tests failing on Ubuntu 11.10 x64
linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):DEFINES+=UBUNTU_ONEIRIC
