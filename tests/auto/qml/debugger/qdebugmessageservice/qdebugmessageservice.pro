CONFIG += testcase
TARGET = tst_qdebugmessageservice
QT += network qml-private testlib
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h

SOURCES +=     tst_qdebugmessageservice.cpp \
            ../shared/debugutil.cpp

INCLUDEPATH += ../shared

include(../../../shared/util.pri)

TESTDATA = data/*

CONFIG += parallel_test

OTHER_FILES += data/test.qml
