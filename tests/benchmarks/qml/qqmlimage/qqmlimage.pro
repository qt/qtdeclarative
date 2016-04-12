CONFIG += benchmark
TEMPLATE = app
TARGET = tst_qqmlimage
QT += qml quick-private testlib
macx:CONFIG -= app_bundle
CONFIG += release

SOURCES += tst_qqmlimage.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

