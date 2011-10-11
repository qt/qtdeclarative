CONFIG += testcase
TARGET = tst_qsgborderimage
macx:CONFIG -= app_bundle

HEADERS += ../shared/testhttpserver.h
SOURCES += tst_qsgborderimage.cpp ../shared/testhttpserver.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private declarative-private network widgets testlib

qpa:CONFIG+=insignificant_test  # QTBUG-21004 fails, unstably
