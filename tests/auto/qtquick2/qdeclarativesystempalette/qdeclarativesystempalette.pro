CONFIG += testcase
TARGET = tst_qdeclarativesystempalette
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativesystempalette.cpp

CONFIG += parallel_test
QT += core-private gui-private declarative-private quick-private widgets testlib
