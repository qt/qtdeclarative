load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui network qtquick1
macx:CONFIG -= app_bundle

HEADERS += ../../declarative/shared/testhttpserver.h
SOURCES += tst_qdeclarativefontloader.cpp ../../declarative/shared/testhttpserver.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private declarative-private qtquick1-private
