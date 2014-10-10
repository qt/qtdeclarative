CONFIG += testcase
TARGET = tst_qqmldebuggingenabler
QT += qml testlib gui-private
osx:CONFIG -= app_bundle

SOURCES +=     tst_qqmldebuggingenabler.cpp

INCLUDEPATH += ../shared
include(../../../shared/util.pri)
include(../shared/debugutil.pri)

OTHER_FILES += data/test.qml
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
