TARGET  = qtquickextras2plugin
TARGETPATH = QtQuick/Extras.2
IMPORT_VERSION = 2.0

QT += qml quick
QT += core-private gui-private qml-private quick-private quickcontrols2-private quickextras2-private

OTHER_FILES += \
    qmldir

QML_FILES = \
    Dial.qml \
    Drawer.qml \
    SwipeView.qml \
    Tumbler.qml

SOURCES += \
    $$PWD/qtquickextras2plugin.cpp

CONFIG += no_cxx_module
load(qml_plugin)

DISTFILES +=
