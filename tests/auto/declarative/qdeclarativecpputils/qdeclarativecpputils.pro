CONFIG += testcase
TARGET = tst_qdeclarativecpputils
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativecpputils.cpp

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
