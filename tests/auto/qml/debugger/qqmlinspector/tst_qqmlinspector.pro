CONFIG += testcase
TARGET = tst_qqmlinspector
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h
SOURCES += tst_qqmlinspector.cpp \
           ../shared/debugutil.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"
CONFIG += parallel_test declarative_debug

QT += qml-private testlib
