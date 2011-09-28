load(qttest_p4)
TEMPLATE = app
TARGET = tst_script
QT += declarative
macx:CONFIG -= app_bundle
CONFIG += release

SOURCES += tst_script.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"
