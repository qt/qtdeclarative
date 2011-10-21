CONFIG += testcase
TEMPLATE = app
TARGET = tst_qdeclarativecomponent
QT += declarative testlib
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativecomponent.cpp testtypes.cpp
HEADERS += testtypes.h

# Define SRCDIR equal to test's source directory
DEFINES += SRCDIR=\\\"$$PWD\\\"
