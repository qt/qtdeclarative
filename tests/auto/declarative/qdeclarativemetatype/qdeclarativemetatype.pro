CONFIG += testcase
TARGET = tst_qdeclarativemetatype
SOURCES += tst_qdeclarativemetatype.cpp
macx:CONFIG -= app_bundle

CONFIG += parallel_test
QT += core-private gui-private declarative-private widgets testlib
