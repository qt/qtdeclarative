CONFIG += testcase
TARGET = tst_qdeclarativevaluetypes
macx:CONFIG -= app_bundle

HEADERS += testtypes.h

SOURCES += tst_qdeclarativevaluetypes.cpp \
           testtypes.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += insignificant_test parallel_test

QT += core-private gui-private v8-private declarative-private testlib
