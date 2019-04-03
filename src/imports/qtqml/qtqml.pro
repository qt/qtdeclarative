TARGETPATH = QtQml
CXX_MODULE = qml
TARGET  = qmlplugin
IMPORT_VERSION = 2.$$QT_MINOR_VERSION

SOURCES += \
    plugin.cpp

# In Qt6 we won't need qmlmodels-private here
QT = qml-private qmlmodels-private

load(qml_plugin)
