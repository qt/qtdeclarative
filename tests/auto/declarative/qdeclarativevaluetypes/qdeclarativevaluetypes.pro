CONFIG += testcase
TARGET = tst_qdeclarativevaluetypes
macx:CONFIG -= app_bundle

HEADERS += testtypes.h \
           ../../shared/util.h

SOURCES += tst_qdeclarativevaluetypes.cpp \
           testtypes.cpp \
           ../../shared/util.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private testlib
