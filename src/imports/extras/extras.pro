TARGET  = qtquickextras2plugin
TARGETPATH = QtQuick/Extras.2
IMPORT_VERSION = 2.0

QT += qml quick
QT += core-private gui-private qml-private quick-private quickcontrols-private quickextras-private

OTHER_FILES += \
    qmldir

QML_FILES = \
    Drawer.qml

SOURCES += \
    $$PWD/qtquickextras2plugin.cpp

CONFIG += no_cxx_module
load(qml_plugin)
