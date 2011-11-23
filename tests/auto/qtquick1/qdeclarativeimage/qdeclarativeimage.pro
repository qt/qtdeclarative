CONFIG += testcase
TARGET = tst_qdeclarativeimage
macx:CONFIG -= app_bundle

HEADERS += ../../shared/testhttpserver.h
SOURCES += tst_qdeclarativeimage.cpp ../../shared/testhttpserver.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private declarative-private qtquick1-private network testlib
