TEMPLATE = app
TARGET = tst_universal
CONFIG += qmltestcase

SOURCES += \
    $$PWD/tst_universal.cpp

OTHER_FILES += \
    $$PWD/data/*

TESTDATA += \
    $$PWD/data/tst_*
