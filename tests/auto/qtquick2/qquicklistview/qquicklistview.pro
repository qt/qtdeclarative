CONFIG += testcase
TARGET = tst_qquicklistview
macx:CONFIG -= app_bundle

HEADERS += incrementalmodel.h \
           ../../shared/util.h
SOURCES += tst_qquicklistview.cpp \
           incrementalmodel.cpp \
           ../../shared/util.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test
QT += core-private gui-private declarative-private quick-private widgets widgets-private v8-private opengl-private testlib
