load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui qtquick1
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativetextinput.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private
