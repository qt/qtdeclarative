CONFIG += testcase
TARGET = tst_qdeclarativefontloader
macx:CONFIG -= app_bundle

HEADERS += ../../declarative/shared/testhttpserver.h
SOURCES += tst_qdeclarativefontloader.cpp ../../declarative/shared/testhttpserver.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private declarative-private network testlib
