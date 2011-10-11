CONFIG += testcase
TEMPLATE = app
TARGET = tst_compilation
QT += declarative testlib
macx:CONFIG -= app_bundle

CONFIG += release

SOURCES += tst_compilation.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"
