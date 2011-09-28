load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative network qtquick1
HEADERS += ../../declarative/shared/testhttpserver.h
SOURCES += tst_qdeclarativeanimatedimage.cpp ../../declarative/shared/testhttpserver.cpp
macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private declarative-private qtquick1-private
