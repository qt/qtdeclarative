CXX_MODULE = qml
TARGET  = modelsplugin
TARGETPATH = QtQml/Models.2
IMPORT_VERSION = 2.$$QT_MINOR_VERSION

SOURCES += \
    plugin.cpp

QT = qml-private qmlmodels-private

load(qml_plugin)
