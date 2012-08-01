CONFIG += testcase
TEMPLATE = app
TARGET = tst_creation
macx:CONFIG -= app_bundle

SOURCES += tst_creation.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private qml-private qtquick1-private widgets testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
