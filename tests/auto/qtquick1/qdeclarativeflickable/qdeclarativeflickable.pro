oad(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui qtquick1
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeflickable.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private
