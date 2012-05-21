CONFIG += testcase
TARGET = tst_qqmlenginedebugservice
macx:CONFIG -= app_bundle

HEADERS += \
    qqmlenginedebugclient.h

SOURCES += \
    tst_qqmlenginedebugservice.cpp \
    qqmlenginedebugclient.cpp

INCLUDEPATH += ../shared
include(../../../shared/util.pri)
include(../shared/debugutil.pri)

DEFINES += QT_QML_DEBUG_NO_WARNING

QT += core-private qml-private quick-private v8-private testlib
