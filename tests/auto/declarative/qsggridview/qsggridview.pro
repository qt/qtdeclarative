load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative
macx:CONFIG -= app_bundle

SOURCES += tst_qsggridview.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
#temporary
CONFIG += insignificant_test
QT += core-private gui-private v8-private declarative-private
QT += opengl-private
