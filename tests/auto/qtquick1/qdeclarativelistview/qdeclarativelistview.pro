CONFIG += testcase
TARGET = tst_qdeclarativelistview
macx:CONFIG -= app_bundle

HEADERS += incrementalmodel.h
SOURCES += tst_qdeclarativelistview.cpp incrementalmodel.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private testlib
