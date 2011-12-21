CONFIG += testcase
TARGET = tst_qdeclarativesqldatabase
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativesqldatabase.cpp \
           ../../shared/util.cpp
HEADERS += ../../shared/util.h

CONFIG += parallel_test

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private v8-private declarative-private quick-private sql testlib
