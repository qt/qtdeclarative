load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui qtquick1
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeparticles.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private declarative-private qtquick1
