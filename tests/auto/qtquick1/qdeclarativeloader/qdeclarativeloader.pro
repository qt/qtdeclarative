CONFIG += testcase
TARGET = tst_qdeclarativeloader
macx:CONFIG -= app_bundle

INCLUDEPATH += ../../declarative/shared/
HEADERS += ../../declarative/shared/testhttpserver.h
SOURCES += tst_qdeclarativeloader.cpp \
           ../../declarative/shared/testhttpserver.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private declarative-private qtquick1-private network testlib
