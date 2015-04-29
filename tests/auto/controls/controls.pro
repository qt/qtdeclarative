TEMPLATE = app
TARGET = tst_controls
CONFIG += qmltestcase

SOURCES += \
    $$PWD/tst_controls.cpp

OTHER_FILES += \
    $$PWD/data/*

TESTDATA += \
    $$PWD/data/tst_*
