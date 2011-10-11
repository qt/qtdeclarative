CONFIG += testcase
TARGET = tst_qdeclarativetextedit
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativetextedit.cpp ../../declarative/shared/testhttpserver.cpp
HEADERS += ../../declarative/shared/testhttpserver.h

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private network testlib
