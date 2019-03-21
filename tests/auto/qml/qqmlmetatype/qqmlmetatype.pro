CONFIG += testcase
TARGET = tst_qqmlmetatype
SOURCES += tst_qqmlmetatype.cpp
macx:CONFIG -= app_bundle

TESTDATA = data/*
include (../../shared/util.pri)

qmlfiles.files = data/CompositeType.qml
qmlfiles.prefix = /tstqqmlmetatype
RESOURCES += qmlfiles

QT += core-private gui-private qml-private testlib
