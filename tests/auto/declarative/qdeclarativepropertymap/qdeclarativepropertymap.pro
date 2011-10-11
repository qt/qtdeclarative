CONFIG += testcase
TARGET = tst_qdeclarativepropertymap
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativepropertymap.cpp

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
