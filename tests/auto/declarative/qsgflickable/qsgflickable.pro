CONFIG += testcase
TARGET = tst_qsgflickable
macx:CONFIG -= app_bundle

SOURCES += tst_qsgflickable.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
#temporary
CONFIG += insignificant_test
QT += core-private gui-private v8-private declarative-private testlib
