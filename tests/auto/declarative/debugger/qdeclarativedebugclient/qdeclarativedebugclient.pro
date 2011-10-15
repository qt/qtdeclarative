CONFIG += testcase
TARGET = tst_qdeclarativedebugclient
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h
SOURCES += tst_qdeclarativedebugclient.cpp \
           ../shared/debugutil.cpp

CONFIG += declarative_debug

QT += core-private gui-private declarative-private network testlib

