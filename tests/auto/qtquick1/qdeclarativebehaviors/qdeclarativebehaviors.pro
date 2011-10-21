CONFIG += testcase
TARGET = tst_qdeclarativebehaviors
SOURCES += tst_qdeclarativebehaviors.cpp
macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test insignificant_test

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private testlib
