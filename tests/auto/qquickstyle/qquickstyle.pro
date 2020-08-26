CONFIG += testcase
TARGET = tst_qquickstyle
SOURCES += tst_qquickstyle.cpp

macos:CONFIG -= app_bundle

QT += quickcontrols2 testlib
QT_PRIVATE += core-private gui-private quickcontrols2-private

include (../shared/util.pri)

TESTDATA = $$PWD/data/*

OTHER_FILES += \
    data/CmdLineArgStyle/Control.qml \
    data/CmdLineArgStyle/qmldir \
    data/EnvVarStyle/Control.qml \
    data/EnvVarStyle/qmldir \
    data/EnvVarFallbackStyle/Control.qml \
    data/EnvVarFallbackStyle/qmldir

custom.files = \
    data/Custom/Label.qml \
    data/Custom/qmldir
custom.prefix = /
RESOURCES += custom
