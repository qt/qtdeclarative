CONFIG += testcase
TARGET = tst_qdeclarativelocale
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativelocale.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += declarative testlib
