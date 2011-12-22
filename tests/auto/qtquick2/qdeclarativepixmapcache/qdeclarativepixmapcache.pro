CONFIG += testcase
TARGET = tst_qdeclarativepixmapcache
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativepixmapcache.cpp \
           ../../shared/testhttpserver.cpp
HEADERS += ../../shared/testhttpserver.h
INCLUDEPATH += ../../shared/

include (../../shared/util.pri)

importFiles.files = data
importFiles.path = .
DEPLOYMENT += importFiles

# QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage
# LIBS += -lgcov

CONFIG += parallel_test

QT += core-private gui-private declarative-private quick-private network testlib
