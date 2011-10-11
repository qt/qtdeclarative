CONFIG += testcase
TEMPLATE = app
TARGET = tst_creation
macx:CONFIG -= app_bundle

SOURCES += tst_creation.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private declarative-private qtquick1-private widgets testlib
