CONFIG += testcase
TARGET = tst_testevent
macos:CONFIG -= app_bundle

SOURCES += tst_testevent.cpp
QT += quick testlib qmltest-private

include (../../shared/util.pri)
