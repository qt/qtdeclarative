CONFIG += testcase
TARGET = tst_qdeclarativeconsole
SOURCES += tst_qdeclarativeconsole.cpp \
           ../../shared/util.cpp
HEADERS += ../../shared/util.h
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += declarative testlib
