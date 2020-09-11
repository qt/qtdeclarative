CONFIG += testcase
TARGET = tst_styleimportscompiletimematerial
SOURCES += tst_styleimportscompiletimematerial.cpp

macos:CONFIG -= app_bundle

QT += testlib core-private gui-private qml-private quick-private quickcontrols2-private quicktemplates2-private

include (../shared/util.pri)

TESTDATA = data/*

OTHER_FILES += \
    data/*.qml

