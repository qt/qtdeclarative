CXX_MODULE = qml
TARGET  = modelsplugin
TARGETPATH = QtQml/Models
IMPORT_VERSION = 2.15

SOURCES += \
    plugin.cpp

QT = qml-private qmlmodels-private

load(qml_plugin)
