CONFIG += testcase qmltypes
QML_IMPORT_NAME = StaticTest
QML_IMPORT_VERSION = 1.0

TARGET = tst_qqmllanguage
macx:CONFIG -= app_bundle

SOURCES += tst_qqmllanguage.cpp \
           testtypes.cpp
HEADERS += testtypes.h

HEADERS += ../../shared/testhttpserver.h
SOURCES += ../../shared/testhttpserver.cpp

TESTDATA = data/*

QT += core-private gui-private  qml-private network testlib

include (../../shared/util.pri)

OTHER_FILES += \
    data/readonlyObjectProperty.qml

android: RESOURCES += qqmllanguage.qrc
