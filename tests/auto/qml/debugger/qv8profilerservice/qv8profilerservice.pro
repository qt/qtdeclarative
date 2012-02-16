CONFIG += testcase
TARGET = tst_qv8profilerservice
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h

SOURCES += tst_qv8profilerservice.cpp \
           ../shared/debugutil.cpp

include (../../../shared/util.pri)

OTHER_FILES += data/test.qml

CONFIG += parallel_test declarative_debug

QT += qml-private testlib
