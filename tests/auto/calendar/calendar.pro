TEMPLATE = app
TARGET = tst_calendar
CONFIG += qmltestcase

SOURCES += \
    $$PWD/tst_calendar.cpp

OTHER_FILES += \
    $$PWD/data/*

TESTDATA += \
    $$PWD/data/tst_*
