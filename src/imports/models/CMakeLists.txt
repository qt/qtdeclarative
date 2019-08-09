# Generated from models.pro.

#####################################################################
## modelsplugin Plugin:
#####################################################################

add_qml_module(modelsplugin
    CPP_PLUGIN
    TARGET_PATH "QtQml/Models.2"
    URI "QtQml.Models"
    VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    RESOURCE_PREFIX "/qt-project.org/imports"
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::QmlModels
)

#### Keys ignored in scope 1:.:.:models.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# TARGETPATH = "QtQml/Models.2"
# _LOADED = "qml_plugin"
