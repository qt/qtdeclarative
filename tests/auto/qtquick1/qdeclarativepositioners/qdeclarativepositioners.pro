CONFIG += testcase
TARGET = tst_qdeclarativepositioners
SOURCES += tst_qdeclarativepositioners.cpp
macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private testlib
