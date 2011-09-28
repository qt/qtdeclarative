load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui network
macx:CONFIG -= app_bundle

INCLUDEPATH += ../shared/
HEADERS += ../shared/testhttpserver.h
SOURCES += tst_qsgloader.cpp \
           ../shared/testhttpserver.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private declarative-private
