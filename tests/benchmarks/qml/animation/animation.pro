TEMPLATE = app
TARGET = tst_animation
QT += qml
macx:CONFIG -= app_bundle

SOURCES += tst_animation.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += testlib core-private gui-private qml-private quick-private v8-private
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
