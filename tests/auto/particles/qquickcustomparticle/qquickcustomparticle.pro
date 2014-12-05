CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qquickcustomparticle
SOURCES += tst_qquickcustomparticle.cpp
macx:CONFIG -= app_bundle

include (../../shared/util.pri)
TESTDATA = data/*

QT += core-private gui-private  qml-private quick-private quickparticles-private testlib

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

# Skip until fix from qtbase 5.4.0 reached dev.
win32: CONFIG += insignificant_test
