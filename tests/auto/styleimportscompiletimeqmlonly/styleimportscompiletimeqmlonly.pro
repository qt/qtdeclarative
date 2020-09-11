CONFIG += testcase
TARGET = tst_styleimportscompiletimeqmlonly
SOURCES += tst_styleimportscompiletimeqmlonly.cpp

macos:CONFIG -= app_bundle

QT += testlib core-private gui-private qml-private quick-private quickcontrols2-private quicktemplates2-private

include (../shared/util.pri)

TESTDATA = data/*

OTHER_FILES += \
    data/*.qml \
    data/QmlOnly/*.qml \
    data/QmlOnly/qmldir

