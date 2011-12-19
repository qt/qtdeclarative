CONFIG += testcase
TARGET = tst_qdeclarativepropertycache
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativepropertycache.cpp

CONFIG += parallel_test
QT += core-private gui-private declarative-private testlib v8-private
