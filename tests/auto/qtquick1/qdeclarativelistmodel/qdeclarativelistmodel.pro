load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative qtquick1
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativelistmodel.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private v8-private widgets-private declarative-private qtquick1-private
