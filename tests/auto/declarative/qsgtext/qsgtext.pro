CONFIG += testcase
TARGET = tst_qsgtext
macx:CONFIG -= app_bundle

SOURCES += tst_qsgtext.cpp

INCLUDEPATH += ../shared/
HEADERS += ../shared/testhttpserver.h
SOURCES += ../shared/testhttpserver.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += insignificant_test parallel_test

QT += core-private gui-private v8-private declarative-private widgets-private opengl-private network testlib
