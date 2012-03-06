CONFIG += testcase
TARGET = tst_qv8profilerservice
macx:CONFIG -= app_bundle

SOURCES += tst_qv8profilerservice.cpp

INCLUDEPATH += ../shared
include(../../../shared/util.pri)
include(../shared/debugutil.pri)

OTHER_FILES += data/test.qml

CONFIG += parallel_test declarative_debug

QT += qml testlib
