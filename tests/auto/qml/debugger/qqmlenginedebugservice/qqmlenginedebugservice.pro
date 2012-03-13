CONFIG += testcase
TARGET = tst_qqmlenginedebugservice
macx:CONFIG -= app_bundle

HEADERS += \
    qqmlenginedebugclient.h

SOURCES += tst_qqmlenginedebugservice.cpp \
    qqmlenginedebugclient.cpp

INCLUDEPATH += ../shared
include(../shared/debugutil.pri)

CONFIG += parallel_test declarative_debug

QT += core-private qml-private quick-private v8-private testlib
