CONFIG += testcase
TARGET = tst_qpacketprotocol
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h
SOURCES += tst_qpacketprotocol.cpp \
           ../shared/debugutil.cpp

CONFIG += parallel_test
QT += core-private gui-private v8-private declarative-private network testlib
