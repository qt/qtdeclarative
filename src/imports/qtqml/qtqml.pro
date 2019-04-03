TARGETPATH = QtQml
CXX_MODULE = qml
TARGET  = qmlplugin
IMPORT_VERSION = 2.$$QT_MINOR_VERSION

SOURCES += \
    plugin.cpp

QT = qml-private

load(qml_plugin)
