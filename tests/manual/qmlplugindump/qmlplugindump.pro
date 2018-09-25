CONFIG += testcase
TARGET = tst_qmlplugindump
QT += testlib gui-private
osx:CONFIG -= app_bundle
CONFIG += parallel_test

DEFINES += QT_TEST_DIR=\\\"$${_PRO_FILE_PWD_}/\\\"

SOURCES += tst_qmlplugindump.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
