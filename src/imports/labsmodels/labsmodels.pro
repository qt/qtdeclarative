CXX_MODULE = qml
TARGET  = labsmodelsplugin
TARGETPATH = Qt/labs/qmlmodels
IMPORT_VERSION = 1.0

SOURCES += \
    plugin.cpp

QT = qml-private qmlmodels-private

load(qml_plugin)
