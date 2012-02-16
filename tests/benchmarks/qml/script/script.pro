CONFIG += testcase
TEMPLATE = app
TARGET = tst_script
QT += qml qml-private testlib v8-private core-private
macx:CONFIG -= app_bundle
CONFIG += release

SOURCES += tst_script.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"
