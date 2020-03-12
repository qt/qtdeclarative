# Generated from workerscript.pro.

#####################################################################
## workerscriptplugin Plugin:
#####################################################################

qt_add_qml_module(workerscriptplugin
    URI "QtQml.WorkerScript"
    VERSION "${CMAKE_PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQmlWorkerScriptPlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::QmlPrivate
        Qt::QmlWorkerScriptPrivate
)

#### Keys ignored in scope 1:.:.:workerscript.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQml/WorkerScript"
