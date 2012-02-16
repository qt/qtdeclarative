CONFIG += testcase
TARGET = tst_qqmldebugclient
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h
SOURCES += tst_qqmldebugclient.cpp \
           ../shared/debugutil.cpp

CONFIG += declarative_debug

QT += qml-private testlib
