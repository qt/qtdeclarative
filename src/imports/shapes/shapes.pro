CXX_MODULE = qml
TARGET  = qmlshapesplugin
TARGETPATH = QtQuick/Shapes
QML_IMPORT_VERSION = $$QT_VERSION

QT = core gui-private qml quick-private quickshapes-private

SOURCES += \
    plugin.cpp \

load(qml_plugin)
