CONFIG += testcase
TARGET = tst_qdeclarativeecmascript
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeecmascript.cpp \
           testtypes.cpp \
           ../shared/testhttpserver.cpp
HEADERS += testtypes.h \
           ../shared/testhttpserver.h
INCLUDEPATH += ../shared

# QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage
# LIBS += -lgcov

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test
QT += core-private gui-private v8-private declarative-private network widgets testlib
