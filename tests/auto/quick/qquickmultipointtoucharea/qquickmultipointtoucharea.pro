TARGET = tst_qquickmultipointtoucharea
CONFIG += testcase
macx:CONFIG -= app_bundle

SOURCES += tst_qquickmultipointtoucharea.cpp

TESTDATA = data/*

QT += core-private gui-private qml-private quick-private testlib
