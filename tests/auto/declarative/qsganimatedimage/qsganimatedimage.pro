CONFIG += testcase
TARGET = tst_qsganimatedimage
HEADERS += ../shared/testhttpserver.h
SOURCES += tst_qsganimatedimage.cpp ../shared/testhttpserver.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private network testlib
