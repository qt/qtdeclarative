CXX_MODULE = qml
TARGET  = modelsplugin
TARGETPATH = QtQml/Models
QML_IMPORT_VERSION = $$QT_VERSION

SOURCES += \
    plugin.cpp

QT = qml-private qmlmodels-private

load(qml_plugin)
