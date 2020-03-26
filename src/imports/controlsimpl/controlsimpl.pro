TARGET = qtquickcontrols2implplugin
TARGETPATH = QtQuick/Controls/impl
IMPORT_VERSION = 2.$$QT_MINOR_VERSION

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private quickcontrols2-private quickcontrols2impl-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtquickcontrols2implplugin.cpp

CONFIG += no_cxx_module
load(qml_plugin)
