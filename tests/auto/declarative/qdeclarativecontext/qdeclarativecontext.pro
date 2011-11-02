CONFIG += testcase
TARGET = tst_qdeclarativecontext
SOURCES += tst_qdeclarativecontext.cpp
macx:CONFIG -= app_bundle

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib v8-private
