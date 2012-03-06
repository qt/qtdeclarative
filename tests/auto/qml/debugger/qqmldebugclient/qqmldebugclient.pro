CONFIG += testcase
TARGET = tst_qqmldebugclient
macx:CONFIG -= app_bundle

HEADERS += ../shared/qqmldebugtestservice.h

SOURCES += tst_qqmldebugclient.cpp \
           ../shared/qqmldebugtestservice.cpp

INCLUDEPATH += ../shared
include(../shared/debugutil.pri)

CONFIG += declarative_debug

QT += qml-private testlib
