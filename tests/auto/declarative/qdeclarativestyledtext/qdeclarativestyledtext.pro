CONFIG += testcase
TARGET = tst_qdeclarativestyledtext
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativestyledtext.cpp

# QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage
# LIBS += -lgcov

CONFIG += parallel_test
QT += core-private gui-private declarative-private network testlib
