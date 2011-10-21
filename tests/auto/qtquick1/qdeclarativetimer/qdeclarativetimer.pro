CONFIG += testcase
TARGET = tst_qdeclarativetimer
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativetimer.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
QT += core-private gui-private widgets-private declarative-private qtquick1-private testlib
