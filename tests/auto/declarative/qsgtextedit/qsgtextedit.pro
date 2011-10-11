CONFIG += testcase
TARGET = tst_qsgtextedit
macx:CONFIG -= app_bundle

SOURCES += tst_qsgtextedit.cpp ../shared/testhttpserver.cpp
HEADERS += ../shared/testhttpserver.h

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private v8-private declarative-private opengl-private network widgets-private testlib
