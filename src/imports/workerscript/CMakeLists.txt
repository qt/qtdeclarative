# Generated from workerscript.pro.

#####################################################################
## workerscriptplugin Plugin:
#####################################################################

add_qml_module(workerscriptplugin
    INSTALL_QML_FILES
    TARGET_PATH "QtQml/WorkerScript.2"
    IMPORT_VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
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
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# TARGETPATH = "QtQml/WorkerScript.2"
# _LOADED = "qml_plugin"
