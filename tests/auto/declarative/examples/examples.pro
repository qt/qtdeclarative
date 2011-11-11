CONFIG += testcase
TARGET = tst_examples
macx:CONFIG -= app_bundle

SOURCES += tst_examples.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
#temporary
CONFIG += insignificant_test #QTBUG-22672
QT += core-private gui-private declarative-private qtquick1-private widgets-private v8-private testlib
