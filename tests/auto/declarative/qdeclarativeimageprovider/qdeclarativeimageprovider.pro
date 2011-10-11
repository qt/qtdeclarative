CONFIG += testcase
TARGET = tst_qdeclarativeimageprovider
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeimageprovider.cpp

# QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage
# LIBS += -lgcov

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private declarative-private network testlib
