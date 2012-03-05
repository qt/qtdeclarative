CONFIG += testcase
TARGET = tst_qqmlmoduleplugin

HEADERS = ../../shared/testhttpserver.h
SOURCES = tst_qqmlmoduleplugin.cpp \
          ../../shared/testhttpserver.cpp
CONFIG -= app_bundle

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

testImportFiles.files = imports
testImportFiles.path = .
DEPLOYMENT += testImportFiles

QT += core-private gui-private qml-private network testlib
