CONFIG += qmltestcase
macos:CONFIG -= app_bundle
TARGET = tst_quicktestwithcomponents

QT += testlib quick

SOURCES += tst_quicktestwithcomponents.cpp

TESTDATA += \
    $$PWD/data/*.qml
