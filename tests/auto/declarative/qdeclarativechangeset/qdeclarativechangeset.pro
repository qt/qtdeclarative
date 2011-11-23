CONFIG += testcase
TARGET = tst_qdeclarativechangeset
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativechangeset.cpp

CONFIG += parallel_test

QT += core-private gui-private declarative-private quick-private testlib
