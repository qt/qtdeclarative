CONFIG += testcase
TARGET = tst_qdeclarativefolderlistmodel
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativefolderlistmodel.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
QT += core-private gui-private declarative-private testlib
