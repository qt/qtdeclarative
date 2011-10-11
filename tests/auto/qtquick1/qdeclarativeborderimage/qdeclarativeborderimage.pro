CONFIG += testcase
TARGET = tst_qdeclarativeborderimage
macx:CONFIG -= app_bundle

HEADERS += ../../declarative/shared/testhttpserver.h
SOURCES += tst_qdeclarativeborderimage.cpp ../../declarative/shared/testhttpserver.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private declarative-private qtquick1-private network testlib
