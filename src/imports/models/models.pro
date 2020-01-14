CXX_MODULE = qml
TARGET  = modelsplugin
TARGETPATH = QtQml/Models.2
IMPORT_VERSION = 2.15

SOURCES += \
    plugin.cpp

QT = qml-private qmlmodels-private

load(qml_plugin)
