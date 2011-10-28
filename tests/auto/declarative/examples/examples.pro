CONFIG += testcase
TARGET = tst_examples
macx:CONFIG -= app_bundle

SOURCES += tst_examples.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
#temporary
CONFIG += insignificant_test
QT += core-private gui-private declarative-private qtquick1-private widgets-private v8-private testlib

CONFIG+=insignificant_test  # QTBUG-20990, aborts
