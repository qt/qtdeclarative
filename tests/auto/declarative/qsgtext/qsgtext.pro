load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui
QT += network
macx:CONFIG -= app_bundle

SOURCES += tst_qsgtext.cpp

INCLUDEPATH += ../shared/
HEADERS += ../shared/testhttpserver.h
SOURCES += ../shared/testhttpserver.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private widgets-private
QT += opengl-private
