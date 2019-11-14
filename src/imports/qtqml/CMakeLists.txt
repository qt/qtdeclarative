# Generated from qtqml.pro.

#####################################################################
## qmlplugin Plugin:
#####################################################################

qt_add_qml_module(qmlplugin
    URI "QtQml"
    VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQmlPlugin
    IMPORTS
        QtQml.Models
        QtQml.WorkerScript
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
        Qt::CorePrivate # special case
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::QmlModels
)

#### Keys ignored in scope 1:.:.:qtqml.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# TARGETPATH = "QtQml"
