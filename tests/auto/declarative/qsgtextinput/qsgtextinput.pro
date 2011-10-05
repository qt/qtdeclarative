load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui
macx:CONFIG -= app_bundle

SOURCES += tst_qsgtextinput.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += insignificant_test
QT += core-private gui-private v8-private declarative-private
QT += opengl-private
