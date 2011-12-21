CONFIG += testcase
TARGET = tst_qdeclarativeerror
SOURCES += tst_qdeclarativeerror.cpp \
           ../../shared/util.cpp
HEADERS += ../../shared/util.h
macx:CONFIG -= app_bundle

CONFIG += parallel_test

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private declarative-private testlib
