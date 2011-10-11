CONFIG += testcase
TARGET = tst_qsgview
macx:CONFIG -= app_bundle

SOURCES += tst_qsgview.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private declarative-private testlib
