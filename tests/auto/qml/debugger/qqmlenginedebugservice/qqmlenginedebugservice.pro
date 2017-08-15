CONFIG += testcase
TARGET = tst_qqmlenginedebugservice
osx:CONFIG -= app_bundle

SOURCES += \
    tst_qqmlenginedebugservice.cpp

include(../shared/qqmlenginedebugclient.pri)
include(../shared/debugutil.pri)

DEFINES += QT_QML_DEBUG_NO_WARNING

QT += core-private qml-private quick-private  testlib gui-private
