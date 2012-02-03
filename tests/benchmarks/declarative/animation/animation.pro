TEMPLATE = app
TARGET = tst_animation
QT += declarative
macx:CONFIG -= app_bundle

SOURCES += tst_animation.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += testlib core-private gui-private declarative-private quick-private v8-private
