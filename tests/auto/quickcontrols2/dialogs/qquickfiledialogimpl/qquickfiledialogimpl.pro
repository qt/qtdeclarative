CONFIG += testcase
TARGET = tst_qquickfiledialogimpl
SOURCES += tst_qquickfiledialogimpl.cpp

macos:CONFIG -= app_bundle

QT += core-private gui-private testlib qml-private quick-private qmltest quicktemplates2-private

include (../../shared/util.pri)

TESTDATA = data/*

OTHER_FILES += \
    data/*.qml
