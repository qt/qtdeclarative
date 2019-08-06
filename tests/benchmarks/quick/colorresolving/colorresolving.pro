TEMPLATE = app
TARGET = tst_colorresolving
CONFIG += qmltestcase

macos:CONFIG -= app_bundle

SOURCES += \
    $$PWD/tst_colorresolving.cpp

OTHER_FILES += \
    $$PWD/data/*.qml

TESTDATA += \
    $$PWD/data/tst_*
