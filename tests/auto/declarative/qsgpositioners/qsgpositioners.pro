CONFIG += testcase
TARGET = tst_qsgpositioners
SOURCES += tst_qsgpositioners.cpp
macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
#temporary
CONFIG += insignificant_test
QT += core-private gui-private v8-private declarative-private opengl-private testlib
