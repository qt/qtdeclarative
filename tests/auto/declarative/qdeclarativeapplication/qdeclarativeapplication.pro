CONFIG += testcase
TARGET = tst_qdeclarativeapplication
macx:CONFIG -= app_bundle
#temporary
CONFIG += insignificant_test
SOURCES += tst_qdeclarativeapplication.cpp
QT += core-private gui-private declarative-private testlib

