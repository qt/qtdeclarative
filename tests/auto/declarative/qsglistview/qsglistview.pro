load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative
macx:CONFIG -= app_bundle

HEADERS += incrementalmodel.h
SOURCES += tst_qsglistview.cpp incrementalmodel.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += insignificant_test parallel_test
QT += core-private gui-private declarative-private widgets widgets-private v8-private
QT += opengl-private
