CONFIG += testcase
TARGET = tst_qqmlincubator
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlincubator.cpp \
           testtypes.cpp

HEADERS += testtypes.h

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private v8-private qml-private network widgets testlib
