CONFIG += testcase
TARGET = tst_qquicklistview
macx:CONFIG -= app_bundle

HEADERS += incrementalmodel.h
SOURCES += tst_qquicklistview.cpp \
           incrementalmodel.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private declarative-private quick-private widgets widgets-private v8-private opengl-private testlib
