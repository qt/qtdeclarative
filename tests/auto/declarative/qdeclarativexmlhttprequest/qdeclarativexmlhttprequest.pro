CONFIG += testcase
TARGET = tst_qdeclarativexmlhttprequest
macx:CONFIG -= app_bundle

INCLUDEPATH += ../../shared/
HEADERS += ../../shared/testhttpserver.h \
           ../../shared/util.h

SOURCES += tst_qdeclarativexmlhttprequest.cpp \
           ../../shared/testhttpserver.cpp \
           ../../shared/util.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private network testlib
