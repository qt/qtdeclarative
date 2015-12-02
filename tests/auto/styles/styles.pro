TEMPLATE = app
TARGET = tst_styles
CONFIG += qmltestcase

DEFINES += TST_CONTROLS_DATA=\\\"$$QQC2_SOURCE_TREE/tests/auto/controls/data\\\"

SOURCES += \
    $$PWD/tst_styles.cpp
