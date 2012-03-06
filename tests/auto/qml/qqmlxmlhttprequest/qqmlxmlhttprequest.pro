CONFIG += testcase
TARGET = tst_qqmlxmlhttprequest
macx:CONFIG -= app_bundle

INCLUDEPATH += ../../shared/
HEADERS += ../../shared/testhttpserver.h

SOURCES += tst_qqmlxmlhttprequest.cpp \
           ../../shared/testhttpserver.cpp

include (../../shared/util.pri)

TESTDATA = data/*

CONFIG += parallel_test

QT += core-private gui-private qml-private network testlib
