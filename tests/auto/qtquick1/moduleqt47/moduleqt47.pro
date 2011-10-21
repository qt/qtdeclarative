CONFIG += testcase
TARGET = tst_moduleqt47
macx:CONFIG -= app_bundle

SOURCES += tst_moduleqt47.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private declarative-private qtquick1-private testlib
