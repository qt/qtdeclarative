CONFIG += testcase
TARGET = tst_qdeclarativegridview
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativegridview.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private testlib
