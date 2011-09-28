load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui widgets
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativesystempalette.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
#temporary
CONFIG += insignificant_test
QT += core-private gui-private declarative-private
