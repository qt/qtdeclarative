CONFIG += testcase
TARGET = tst_qquicktimer
macx:CONFIG -= app_bundle

SOURCES += tst_qquicktimer.cpp

CONFIG += parallel_test
QT += core-private gui-private qml-private quick-private gui testlib
