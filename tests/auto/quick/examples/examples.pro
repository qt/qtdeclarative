CONFIG += testcase
testcase.timeout = 400 # this test is slow
TARGET = tst_examples
macx:CONFIG -= app_bundle

SOURCES += tst_examples.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
#temporary
QT += core-private gui-private qml-private quick-private  testlib
!qtHaveModule(xmlpatterns): DEFINES += QT_NO_XMLPATTERNS

