load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui network
macx:CONFIG -= app_bundle

SOURCES += tst_qsgdroparea.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private declarative-private
