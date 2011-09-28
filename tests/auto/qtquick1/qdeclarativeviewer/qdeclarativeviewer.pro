load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui qtquick1
macx:CONFIG -= app_bundle

include(../../../../tools/qmlviewer/qml.pri)

SOURCES += tst_qdeclarativeviewer.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test insignificant_test
QT += core-private gui-private widgets-private declarative-private qtquick1-private widgets-private v8-private
