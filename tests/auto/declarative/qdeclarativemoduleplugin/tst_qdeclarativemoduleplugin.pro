CONFIG += testcase
TARGET = tst_qdeclarativemoduleplugin

HEADERS = ../../shared/testhttpserver.h \
          ../../shared/util.h
SOURCES = tst_qdeclarativemoduleplugin.cpp \
          ../../shared/testhttpserver.cpp \
          ../../shared/util.cpp
CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

testImportFiles.files = imports
testImportFiles.path = .
DEPLOYMENT += testImportFiles

QT += core-private gui-private declarative-private network testlib
