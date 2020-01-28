# Generated from workerscript.pro.

#####################################################################
## workerscriptplugin Plugin:
#####################################################################

qt_add_qml_module(workerscriptplugin
    URI "QtQml.WorkerScript"
    VERSION "2.15"
    DESIGNER_SUPPORTED
    CLASSNAME QtQmlWorkerScriptPlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::QmlPrivate
        Qt::QmlWorkerScriptPrivate
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::QmlWorkerScript
)

#### Keys ignored in scope 1:.:.:workerscript.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "2.15"
# TARGETPATH = "QtQml/WorkerScript"
