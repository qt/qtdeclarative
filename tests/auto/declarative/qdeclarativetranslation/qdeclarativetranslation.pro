CONFIG += testcase
TARGET = tst_qdeclarativetranslation
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativetranslation.cpp
RESOURCES += data/translation.qrc

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
