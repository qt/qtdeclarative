CONFIG += testcase
TARGET = tst_qdeclarativelayoutitem
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativelayoutitem.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private widgets-private declarative-private qtquick1-private testlib
