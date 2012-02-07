CONFIG += testcase
TARGET = tst_qdeclarativemetaobject
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativemetaobject.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test
QT += declarative testlib
