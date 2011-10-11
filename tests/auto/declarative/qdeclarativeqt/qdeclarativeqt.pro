CONFIG += testcase
TARGET = tst_qdeclarativeqt
SOURCES += tst_qdeclarativeqt.cpp
macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

# QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage
# LIBS += -lgcov

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private testlib
