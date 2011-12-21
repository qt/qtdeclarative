CONFIG += testcase
TARGET = tst_qdeclarativecomponent
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativecomponent.cpp \
           ../../shared/util.cpp
HEADERS += ../../shared/util.h

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private network testlib
