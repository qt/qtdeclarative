CONFIG += testcase
TARGET = tst_qdeclarativetext
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativetext.cpp

INCLUDEPATH += ../../declarative/shared/
HEADERS += ../../declarative/shared/testhttpserver.h
SOURCES += ../../declarative/shared/testhttpserver.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private network testlib
