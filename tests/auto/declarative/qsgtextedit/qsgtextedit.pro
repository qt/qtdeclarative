CONFIG += testcase
TARGET = tst_qsgtextedit
macx:CONFIG -= app_bundle

SOURCES += tst_qsgtextedit.cpp ../shared/testhttpserver.cpp
HEADERS += ../shared/testhttpserver.h

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private v8-private declarative-private opengl-private network widgets-private testlib
