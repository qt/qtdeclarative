CONFIG += testcase
TARGET = tst_qdeclarativefolderlistmodel
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativefolderlistmodel.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test
QT += core-private gui-private declarative-private testlib
