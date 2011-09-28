load(qttest_p4)
TEMPLATE = app
TARGET = tst_creation
QT += declarative qtquick1 widgets
macx:CONFIG -= app_bundle

SOURCES += tst_creation.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private declarative-private qtquick1-private
