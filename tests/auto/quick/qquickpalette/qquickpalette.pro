CONFIG += testcase
TARGET = tst_qquickpalette
SOURCES += tst_qquickpalette.cpp

macos:CONFIG -= app_bundle

QT += core-private qml-private quick-private testlib
