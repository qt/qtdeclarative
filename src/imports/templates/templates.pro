TARGET  = qtquicktemplates2plugin
TARGETPATH = QtQuick/Templates.2
IMPORT_VERSION = 2.0

QT += qml quick
QT += core-private gui-private qml-private quick-private quickcontrols2-private

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtquicktemplates2plugin.cpp

CONFIG += no_cxx_module
load(qml_plugin)
