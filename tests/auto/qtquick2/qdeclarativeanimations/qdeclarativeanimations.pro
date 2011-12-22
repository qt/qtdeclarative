CONFIG += testcase
TARGET = tst_qdeclarativeanimations
SOURCES += tst_qdeclarativeanimations.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private quick-private opengl-private testlib
