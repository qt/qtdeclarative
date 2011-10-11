CONFIG += testcase
TARGET = tst_qdeclarativemetatype
SOURCES += tst_qdeclarativemetatype.cpp
macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
#temporary
CONFIG += insignificant_test
QT += core-private gui-private declarative-private widgets testlib
