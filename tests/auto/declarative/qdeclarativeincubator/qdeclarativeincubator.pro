CONFIG += testcase
TARGET = tst_qdeclarativeincubator
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeincubator.cpp \
           testtypes.cpp
HEADERS += testtypes.h

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private network widgets testlib
