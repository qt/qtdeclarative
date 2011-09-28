load(qttest_p4)

HEADERS = ../shared/testhttpserver.h
SOURCES = tst_qdeclarativemoduleplugin.cpp \
          ../shared/testhttpserver.cpp
QT += declarative network
CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private declarative-private
