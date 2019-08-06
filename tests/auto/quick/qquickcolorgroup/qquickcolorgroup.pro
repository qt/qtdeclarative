CONFIG += testcase
TARGET = tst_qquickcolorgroup
SOURCES += tst_qquickcolorgroup.cpp

macos:CONFIG -= app_bundle

QT += core-private qml-private quick-private testlib
