CONFIG += testcase
TEMPLATE = app
TARGET = tst_script
QT += declarative testlib
macx:CONFIG -= app_bundle
CONFIG += release

SOURCES += tst_script.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"
