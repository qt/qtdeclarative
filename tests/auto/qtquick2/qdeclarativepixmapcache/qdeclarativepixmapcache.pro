CONFIG += testcase
TARGET = tst_qdeclarativepixmapcache
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativepixmapcache.cpp

INCLUDEPATH += ../../shared/
HEADERS += ../../shared/testhttpserver.h
SOURCES += ../../shared/testhttpserver.cpp

importFiles.files = data
importFiles.path = .
DEPLOYMENT += importFiles

# QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage
# LIBS += -lgcov

CONFIG += parallel_test

QT += core-private gui-private declarative-private quick-private network testlib
