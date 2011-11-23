CONFIG += testcase
TARGET = tst_nodestest
macx:CONFIG   -= app_bundle

SOURCES += tst_nodestest.cpp

CONFIG+=parallel_test

QT += core-private gui-private declarative-private quick-private opengl widgets testlib
