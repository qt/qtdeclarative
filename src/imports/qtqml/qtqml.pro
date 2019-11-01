TARGETPATH = QtQml
CXX_MODULE = qml
TARGET  = qmlplugin
IMPORT_VERSION = 2.$$QT_MINOR_VERSION

SOURCES += \
    plugin.cpp

# In Qt6 we won't need qmlmodels-private here
QT = qml-private qmlmodels-private

DYNAMIC_QMLDIR = \
    "module QtQml" \
    "plugin qmlplugin" \
    "classname QtQmlPlugin" \
    "typeinfo plugins.qmltypes" \
    "designersupported" \
    "import QtQml.Models"

qtConfig(qml-worker-script): DYNAMIC_QMLDIR += \
    "import QtQml.WorkerScript"

load(qml_plugin)
