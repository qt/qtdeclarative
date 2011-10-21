CONFIG += testcase
TEMPLATE = app
TARGET = tst_binding
QT += declarative testlib
macx:CONFIG -= app_bundle

SOURCES += tst_binding.cpp testtypes.cpp
HEADERS += testtypes.h

# Define SRCDIR equal to test's source directory
DEFINES += SRCDIR=\\\"$$PWD\\\"
