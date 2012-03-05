CONFIG += testcase
TARGET = tst_qqmlprofilerservice
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h

SOURCES += tst_qqmlprofilerservice.cpp \
           ../shared/debugutil.cpp
OTHER_FILES += data/test.qml

include (../../../shared/util.pri)

CONFIG += parallel_test declarative_debug

QT += core-private v8-private qml-private testlib
