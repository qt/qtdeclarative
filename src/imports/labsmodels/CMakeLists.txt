# Generated from labsmodels.pro.

#####################################################################
## labsmodelsplugin Plugin:
#####################################################################

add_qml_module(labsmodelsplugin
    CPP_PLUGIN
    URI "Qt.labs.qmlmodels"
    VERSION "1.0"
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::QmlModels
)

#### Keys ignored in scope 1:.:.:labsmodels.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.0"
# TARGETPATH = "Qt/labs/qmlmodels"
# _LOADED = "qml_plugin"
