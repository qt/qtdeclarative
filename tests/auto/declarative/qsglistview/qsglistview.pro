CONFIG += testcase
TARGET = tst_qsglistview
macx:CONFIG -= app_bundle

HEADERS += incrementalmodel.h
SOURCES += tst_qsglistview.cpp incrementalmodel.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += insignificant_test parallel_test
QT += core-private gui-private declarative-private widgets widgets-private v8-private opengl-private testlib
