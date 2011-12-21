CONFIG += testcase
TARGET = tst_qdeclarativeinfo
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeinfo.cpp \
           ../../shared/util.cpp
HEADERS += ../../shared/util.h

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test
QT += core-private gui-private declarative-private widgets testlib
