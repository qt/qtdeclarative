CONFIG += testcase
TEMPLATE = app
TARGET = tst_qdeclarativemetaproperty
QT += declarative testlib
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativemetaproperty.cpp 

# Define SRCDIR equal to test's source directory
DEFINES += SRCDIR=\\\"$$PWD\\\"
