TEMPLATE = app
TARGET = tst_qquickuniversalstyle
CONFIG += qmltestcase

SOURCES += \
    $$PWD/tst_qquickuniversalstyle.cpp

RESOURCES += \
    $$PWD/qquickuniversalstyle.qrc

OTHER_FILES += \
    $$PWD/data/*

TESTDATA += \
    $$PWD/data/tst_*
