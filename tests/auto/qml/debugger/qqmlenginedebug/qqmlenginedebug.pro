CONFIG += testcase
TARGET = tst_qqmlenginedebug
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h
SOURCES += tst_qqmlenginedebug.cpp \
           ../shared/debugutil.cpp

CONFIG += parallel_test declarative_debug

QT += core-private qml-private quick-private v8-private testlib
