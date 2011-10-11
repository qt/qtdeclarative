CONFIG += testcase
TARGET = tst_qdeclarativeparticles
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeparticles.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private declarative-private qtquick1 testlib
