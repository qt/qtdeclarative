CONFIG += testcase
TARGET = tst_qdeclarativedebugtrace
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h

SOURCES += tst_qdeclarativedebugtrace.cpp \
           ../shared/debugutil.cpp
OTHER_FILES += data/test.qml

include (../../../shared/util.pri)

CONFIG += parallel_test declarative_debug

QT += declarative-private testlib
