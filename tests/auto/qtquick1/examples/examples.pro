load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative qtquick1
macx:CONFIG -= app_bundle

include(../../../../tools/qmlviewer/qml.pri)

SOURCES += tst_examples.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private declarative-private qtquick1-private

qpa:CONFIG+=insignificant_test  # QTBUG-20990, aborts
