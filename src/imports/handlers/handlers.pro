CXX_MODULE = qml
TARGET  = handlersplugin
TARGETPATH = Qt/labs/handlers
IMPORT_VERSION = 1.0

SOURCES += \
    plugin.cpp

QT += quick-private qml-private

load(qml_plugin)
