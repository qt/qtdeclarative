CONFIG += testcase
TARGET = tst_qquicktext
macx:CONFIG -= app_bundle

SOURCES += tst_qquicktext.cpp

INCLUDEPATH += ../../shared/
HEADERS += ../../shared/testhttpserver.h
SOURCES += ../../shared/testhttpserver.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private quick-private widgets-private opengl-private network testlib
