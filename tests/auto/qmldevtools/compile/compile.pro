CONFIG += testcase
TARGET = tst_compile
QT = core qmldevtools-private testlib
macx:CONFIG -= app_bundle

SOURCES += tst_compile.cpp

CONFIG += parallel_test
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
