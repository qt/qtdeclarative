CONFIG += testcase
TARGET = tst_qqmldebugclient
macx:CONFIG -= app_bundle

SOURCES += tst_qqmldebugclient.cpp

INCLUDEPATH += ../shared
include(../shared/debugutil.pri)

CONFIG += declarative_debug

QT += qml-private testlib
