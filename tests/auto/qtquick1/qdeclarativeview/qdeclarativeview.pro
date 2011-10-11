CONFIG += testcase
TARGET = tst_qdeclarativeview
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeview.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private widgets-private declarative-private qtquick1-private widgets testlib
