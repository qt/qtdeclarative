CONFIG += testcase
TARGET = tst_qdeclarativelistcompositor
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativelistcompositor.cpp

CONFIG += parallel_test

QT += core-private gui-private declarative-private quick-private testlib
