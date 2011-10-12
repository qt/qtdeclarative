CONFIG += testcase
TARGET = tst_qdeclarativev4
macx:CONFIG -= app_bundle

SOURCES += tst_v4.cpp \
           testtypes.cpp 
HEADERS += testtypes.h 

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private network testlib
