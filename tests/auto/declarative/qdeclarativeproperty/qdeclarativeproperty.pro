CONFIG += testcase
TARGET = tst_qdeclarativeproperty
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeproperty.cpp \
           ../../shared/util.cpp
HEADERS += ../../shared/util.h

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private widgets testlib
