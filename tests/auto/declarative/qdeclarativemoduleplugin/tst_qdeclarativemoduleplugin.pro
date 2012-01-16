CONFIG += testcase
TARGET = tst_qdeclarativemoduleplugin

HEADERS = ../../shared/testhttpserver.h
SOURCES = tst_qdeclarativemoduleplugin.cpp \
          ../../shared/testhttpserver.cpp
CONFIG -= app_bundle

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

testImportFiles.files = imports
testImportFiles.path = .
DEPLOYMENT += testImportFiles

QT += core-private gui-private declarative-private network testlib
