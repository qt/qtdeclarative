CONFIG += testcase
TARGET = tst_examples
macx:CONFIG -= app_bundle

include(../../../../tools/qmlviewer/qml.pri)

SOURCES += tst_examples.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private declarative-private qtquick1-private testlib

qpa:CONFIG+=insignificant_test  # QTBUG-20990, aborts
