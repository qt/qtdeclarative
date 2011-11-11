CONFIG += testcase
TARGET = tst_qv8profilerservice
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h

SOURCES += tst_qv8profilerservice.cpp \
           ../shared/debugutil.cpp

OTHER_FILES += data/test.qml

CONFIG += parallel_test declarative_debug

QT += core-private gui-private v8-private declarative-private network testlib
