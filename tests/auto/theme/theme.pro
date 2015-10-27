TEMPLATE = app
TARGET = tst_theme
CONFIG += qmltestcase

SOURCES += \
    $$PWD/tst_theme.cpp

OTHER_FILES += \
    $$PWD/data/*

TESTDATA += \
    $$PWD/data/tst_*
