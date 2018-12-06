CONFIG += qmltestcase
macos:CONFIG -= app_bundle
TARGET = tst_polish-qml

QT += testlib quick quick-private

include (../../shared/util.pri)

SOURCES += tst_polish-qml.cpp

TESTDATA += \
    $$PWD/data/*.qml
