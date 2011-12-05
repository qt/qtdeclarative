CONFIG += testcase
TARGET = tst_qdeclarativeimageprovider
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeimageprovider.cpp

CONFIG += parallel_test

QT += core-private gui-private declarative-private quick-private network testlib
