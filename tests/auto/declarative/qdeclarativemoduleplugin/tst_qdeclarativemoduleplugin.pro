CONFIG += testcase
TARGET = tst_qdeclarativemoduleplugin

HEADERS = ../shared/testhttpserver.h
SOURCES = tst_qdeclarativemoduleplugin.cpp \
          ../shared/testhttpserver.cpp
CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private declarative-private network testlib
