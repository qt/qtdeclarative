CONFIG += testcase
TARGET = tst_qquicktextinput
macx:CONFIG -= app_bundle

SOURCES += tst_qquicktextinput.cpp \
          ../../shared/util.cpp
HEADERS += ../../shared/util.h

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private v8-private declarative-private quick-private opengl-private testlib
