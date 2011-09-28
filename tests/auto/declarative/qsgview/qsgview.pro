load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui
macx:CONFIG -= app_bundle

SOURCES += tst_qsgview.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private declarative-private
