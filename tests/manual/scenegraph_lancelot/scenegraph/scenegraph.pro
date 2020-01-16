CONFIG += testcase
TARGET = tst_scenegraph
DESTDIR=..
macx:CONFIG -= app_bundle
CONFIG += console

SOURCES += tst_scenegraph.cpp

# Include Lancelot protocol code to communicate with baseline server.
# Assuming that we are in a normal Qt5 source code tree
include(../../../../../qtbase/tests/baselineserver/shared/qbaselinetest.pri)

win32: TEST_HELPER_INSTALLS += .././qmlscenegrabber.exe
else: macos: TEST_HELPER_INSTALLS += .././qmlscenegrabber.app
else: TEST_HELPER_INSTALLS += .././qmlscenegrabber

TESTDATA += ../data
