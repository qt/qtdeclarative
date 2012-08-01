CONFIG += testcase
TARGET = tst_qquicklistcompositor
macx:CONFIG -= app_bundle

SOURCES += tst_qquicklistcompositor.cpp

CONFIG += parallel_test

QT += core-private gui-private qml-private quick-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
