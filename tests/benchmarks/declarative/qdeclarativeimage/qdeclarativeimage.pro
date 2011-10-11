CONFIG += testcase
TEMPLATE = app
TARGET = tst_qdeclarativeimage
QT += declarative testlib
macx:CONFIG -= app_bundle
CONFIG += release

SOURCES += tst_qdeclarativeimage.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

