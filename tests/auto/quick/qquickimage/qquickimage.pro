CONFIG += testcase
TARGET = tst_qquickimage
macx:CONFIG -= app_bundle

HEADERS += ../../shared/testhttpserver.h
SOURCES += tst_qquickimage.cpp \
           ../../shared/testhttpserver.cpp

include (../../shared/util.pri)
include (../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test
QT += core-private gui-private qml-private quick-private network testlib
