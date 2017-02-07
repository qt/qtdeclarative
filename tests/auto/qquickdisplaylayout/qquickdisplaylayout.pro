CONFIG += testcase
macos:CONFIG -= app_bundle
TARGET = tst_qquickdisplaylayout

QT += core gui qml quick testlib
QT_PRIVATE += quick-private quickcontrols2-private

include (../shared/util.pri)

SOURCES += tst_qquickdisplaylayout.cpp

TESTDATA += \
    $$PWD/data/*.qml
