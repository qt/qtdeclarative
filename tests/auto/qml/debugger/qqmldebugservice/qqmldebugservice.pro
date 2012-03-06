CONFIG += testcase
TARGET = tst_qqmldebugservice
macx:CONFIG -= app_bundle

SOURCES += tst_qqmldebugservice.cpp
INCLUDEPATH += ../shared
include(../../../shared/util.pri)
include(../shared/debugutil.pri)

CONFIG += parallel_test declarative_debug

QT += qml-private testlib
