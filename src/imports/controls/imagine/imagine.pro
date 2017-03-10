TARGET = qtquickcontrols2imaginestyleplugin
TARGETPATH = QtQuick/Controls.2/Imagine
IMPORT_VERSION = 2.3

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtquickcontrols2imaginestyleplugin.cpp

include(imagine.pri)

qtquickcontrols2imaginestyle.prefix = qt-project.org/imports/QtQuick/Controls.2/Imagine
qtquickcontrols2imaginestyle.files += \
    $$files($$PWD/images/*.png) \
    $$files($$PWD/images/*.webp)
RESOURCES += qtquickcontrols2imaginestyle

CONFIG += no_cxx_module
load(qml_plugin)

requires(qtConfig(quickcontrols2-imagine))
