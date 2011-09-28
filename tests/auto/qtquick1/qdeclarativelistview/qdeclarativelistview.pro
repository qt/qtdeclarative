load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative qtquick1
macx:CONFIG -= app_bundle

HEADERS += incrementalmodel.h
SOURCES += tst_qdeclarativelistview.cpp incrementalmodel.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private
