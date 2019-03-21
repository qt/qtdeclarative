CONFIG += qmltestcase
macos:CONFIG -= app_bundle
TARGET = tst_polish

QT += testlib quick quick-private

include (../../shared/util.pri)

SOURCES += tst_polish.cpp

TESTDATA += \
    $$PWD/data/*.qml
