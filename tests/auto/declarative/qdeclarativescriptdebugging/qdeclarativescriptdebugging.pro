CONFIG += testcase
TARGET = tst_qdeclarativescriptdebugging
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativescriptdebugging.cpp
INCLUDEPATH += ../shared

# QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage
# LIBS += -lgcov

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
