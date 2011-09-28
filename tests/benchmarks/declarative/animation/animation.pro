load(qttest_p4)
TEMPLATE = app
TARGET = tst_animation
QT += declarative
macx:CONFIG -= app_bundle

SOURCES += tst_animation.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private declarative-private
