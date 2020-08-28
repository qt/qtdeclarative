CONFIG += testcase
TARGET = tst_styleimports
SOURCES += tst_styleimports.cpp

macos:CONFIG -= app_bundle

QT += core-private gui-private qml-private quick-private quickcontrols2-private quickcontrols2impl-private testlib

include (../shared/util.pri)

resourcestyle.prefix = /
resourcestyle.base = resources
resourcestyle.files += \
    $$PWD/resources/ResourceStyle/Button.qml \
    $$PWD/resources/ResourceStyle/qmldir
RESOURCES += resourcestyle

TESTDATA = data/*

OTHER_FILES += \
    data/*.qml \
    data/qmldir \
    data/FileSystemStyle/*.qml \
    data/FileSystemStyle/qmldir \
    data/PlatformStyle/*.qml \
    data/PlatformStyle/+linux/*.qml \
    data/PlatformStyle/+macos/*.qml \
    data/PlatformStyle/+windows/*.qml \
    data/PlatformStyle/qmldir

