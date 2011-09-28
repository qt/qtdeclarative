load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative
SOURCES += tst_qsgpositioners.cpp
macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
#temporary
CONFIG += insignificant_test
QT += core-private gui-private v8-private declarative-private
QT += opengl-private
