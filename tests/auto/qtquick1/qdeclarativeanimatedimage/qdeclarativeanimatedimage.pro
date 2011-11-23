CONFIG += testcase
TARGET = tst_qdeclarativeanimatedimage
HEADERS += ../../shared/testhttpserver.h
SOURCES += tst_qdeclarativeanimatedimage.cpp ../../shared/testhttpserver.cpp
macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private declarative-private qtquick1-private network testlib
