CONFIG += testcase
TARGET = tst_qquickfontloader
macx:CONFIG -= app_bundle

HEADERS += ../../shared/testhttpserver.h
SOURCES += tst_qquickfontloader.cpp \
           ../../shared/testhttpserver.cpp

include (../../shared/util.pri)

TESTDATA = data/*

CONFIG += parallel_test

QT += core-private gui-private qml-private quick-private network testlib

win32:CONFIG += insignificant_test # QTBUG-24782
