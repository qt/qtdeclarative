CONFIG += benchmark
TEMPLATE = app
TARGET = tst_affectors
SOURCES += tst_affectors.cpp
macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += quickparticles-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
