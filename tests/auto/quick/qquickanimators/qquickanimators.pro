QT += core-private gui-private qml-private
TEMPLATE=app
TARGET=tst_qquickanimators

CONFIG += qmltestcase
SOURCES += tst_qquickanimators.cpp

TESTDATA = data/*

OTHER_FILES += \
    data/tst_scale.qml \
    data/Scale.qml \
    tst_on.qml \
    data/tst_nested.qml
