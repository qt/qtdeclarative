CONFIG += testcase
TARGET = tst_qquickloader
macx:CONFIG -= app_bundle

INCLUDEPATH += ../../shared/
HEADERS += ../../shared/testhttpserver.h

SOURCES += tst_qquickloader.cpp \
           ../../shared/testhttpserver.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private quick-private network testlib
