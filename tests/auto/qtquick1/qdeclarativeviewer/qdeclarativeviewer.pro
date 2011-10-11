CONFIG += testcase
TARGET = tst_qdeclarativeviewer
macx:CONFIG -= app_bundle

include(../../../../tools/qmlviewer/qml.pri)

SOURCES += tst_qdeclarativeviewer.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test insignificant_test
QT += core-private gui-private widgets-private declarative-private qtquick1-private v8-private testlib
