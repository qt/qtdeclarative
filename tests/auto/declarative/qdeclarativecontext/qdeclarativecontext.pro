CONFIG += testcase
TARGET = tst_qdeclarativecontext
SOURCES += tst_qdeclarativecontext.cpp \
           ../../shared/util.cpp
HEADERS += ../../shared/util.h
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib v8-private
