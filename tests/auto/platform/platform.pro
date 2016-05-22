TEMPLATE = app
TARGET = tst_platform
CONFIG += qmltestcase

SOURCES += \
    $$PWD/tst_platform.cpp

OTHER_FILES += \
    $$PWD/data/*

TESTDATA += \
    $$PWD/data/tst_*
