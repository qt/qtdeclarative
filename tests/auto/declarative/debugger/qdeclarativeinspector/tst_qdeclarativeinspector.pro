CONFIG += testcase
TARGET = tst_qdeclarativeinspector
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h
SOURCES += tst_qdeclarativeinspector.cpp \
           ../shared/debugutil.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"
CONFIG += parallel_test declarative_debug

QT += core-private gui-private v8-private declarative-private network testlib
