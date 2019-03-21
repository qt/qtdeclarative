CONFIG += testcase
TARGET = tst_qquickbehaviors
SOURCES += tst_qquickbehaviors.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

TESTDATA = data/*
DISTFILES = $$files(data/*)
QT += core-private gui-private  qml-private quick-private testlib
