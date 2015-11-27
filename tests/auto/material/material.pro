TEMPLATE = app
TARGET = tst_material
CONFIG += qmltestcase

SOURCES += \
    $$PWD/tst_material.cpp

OTHER_FILES += \
    $$PWD/data/*

TESTDATA += \
    $$PWD/data/tst_*
