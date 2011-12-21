CONFIG += testcase
TARGET = tst_qquickfocusscope
SOURCES += tst_qquickfocusscope.cpp \
           ../../shared/util.cpp
HEADERS += ../../shared/util.h

macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private declarative-private quick-private testlib
