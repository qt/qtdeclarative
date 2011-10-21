CONFIG += testcase
TARGET = tst_qmlmin
QT += declarative testlib
macx:CONFIG -= app_bundle

SOURCES += tst_qmlmin.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
