CONFIG += testcase
TARGET = tst_qsgtextinput
macx:CONFIG -= app_bundle

SOURCES += tst_qsgtextinput.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += insignificant_test
QT += core-private gui-private v8-private declarative-private opengl-private testlib
