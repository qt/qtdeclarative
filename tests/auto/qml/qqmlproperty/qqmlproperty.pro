CONFIG += testcase qmltypes
TARGET = tst_qqmlproperty
macx:CONFIG -= app_bundle

QML_IMPORT_NAME = io.qt.bugreports
QML_IMPORT_VERSION = 2.0

SOURCES += tst_qqmlproperty.cpp

include (../../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private  qml-private testlib

HEADERS += \
    interfaces.h
