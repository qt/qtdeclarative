CONFIG += testcase
TARGET = tst_qqmlxmlhttprequest
macx:CONFIG -= app_bundle

CONFIG+=insignificant_test # QTQAINFRA-573

INCLUDEPATH += ../../shared/
HEADERS += ../../shared/testhttpserver.h

SOURCES += tst_qqmlxmlhttprequest.cpp \
           ../../shared/testhttpserver.cpp

include (../../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private qml-private network testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
