CONFIG += testcase
TARGET = tst_qsgloader
macx:CONFIG -= app_bundle

INCLUDEPATH += ../shared/
HEADERS += ../shared/testhttpserver.h
SOURCES += tst_qsgloader.cpp \
           ../shared/testhttpserver.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private declarative-private network testlib
