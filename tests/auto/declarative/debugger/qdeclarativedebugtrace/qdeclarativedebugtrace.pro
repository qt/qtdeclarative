CONFIG += testcase
TARGET = tst_qdeclarativedebugtrace
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h \
           ../../../shared/util.h

SOURCES += tst_qdeclarativedebugtrace.cpp \
           ../shared/debugutil.cpp \
           ../../../shared/util.cpp

OTHER_FILES += data/test.qml

CONFIG += parallel_test declarative_debug

QT += declarative-private testlib
