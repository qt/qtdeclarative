CONFIG += testcase
TARGET = tst_qdeclarativetextinput
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativetextinput.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private testlib
