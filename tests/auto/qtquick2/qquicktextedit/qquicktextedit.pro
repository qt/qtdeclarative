CONFIG += testcase
TARGET = tst_qquicktextedit
macx:CONFIG -= app_bundle

SOURCES += tst_qquicktextedit.cpp \
           ../../shared/testhttpserver.cpp \
           ../../shared/util.cpp

HEADERS += ../../shared/testhttpserver.h \
           ../../shared/util.h

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private v8-private declarative-private quick-private opengl-private network widgets-private testlib
