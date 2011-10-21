CONFIG += testcase
TARGET = tst_qdeclarativebinding
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativebinding.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private declarative-private qtquick1-private testlib
