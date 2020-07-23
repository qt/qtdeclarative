CONFIG += testcase qmltypes
TARGET = tst_qqmlinfo
macx:CONFIG -= app_bundle

QML_IMPORT_NAME = org.qtproject.Test
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += \
    attached.h

SOURCES += \
    attached.cpp \
    tst_qqmlinfo.cpp

include (../../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private qml-private testlib
