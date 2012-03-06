CONFIG += testcase
TARGET = tst_qqmlprofilerservice
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlprofilerservice.cpp

INCLUDEPATH += ../shared
include(../../../shared/util.pri)
include(../shared/debugutil.pri)

OTHER_FILES += data/test.qml

CONFIG += parallel_test declarative_debug

QT += core-private v8-private qml-private testlib
