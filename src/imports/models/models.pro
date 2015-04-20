CXX_MODULE = qml
TARGET  = modelsplugin
TARGETPATH = QtQml/Models.2
IMPORT_VERSION = 2.2

SOURCES += \
    plugin.cpp

QT = qml-private

load(qml_plugin)
