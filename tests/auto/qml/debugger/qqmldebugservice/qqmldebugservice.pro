CONFIG += testcase
TARGET = tst_qqmldebugservice
macx:CONFIG -= app_bundle

HEADERS += ../shared/qqmldebugtestservice.h

SOURCES += tst_qqmldebugservice.cpp \
           ../shared/qqmldebugtestservice.cpp

INCLUDEPATH += ../shared
include(../../../shared/util.pri)
include(../shared/debugutil.pri)

CONFIG += parallel_test declarative_debug

QT += qml-private testlib
