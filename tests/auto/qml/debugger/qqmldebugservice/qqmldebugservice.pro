CONFIG += testcase
TARGET = tst_qqmldebugservice
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h
SOURCES += tst_qqmldebugservice.cpp \
           ../shared/debugutil.cpp

CONFIG += parallel_test declarative_debug

QT += qml-private testlib
