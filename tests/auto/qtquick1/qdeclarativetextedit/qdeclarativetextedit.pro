CONFIG += testcase
TARGET = tst_qdeclarativetextedit
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativetextedit.cpp ../../shared/testhttpserver.cpp
HEADERS += ../../shared/testhttpserver.h

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private network testlib
