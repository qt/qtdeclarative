# Generated from labsmodels.pro.

#####################################################################
## labsmodelsplugin Plugin:
#####################################################################

qt_internal_add_qml_module(labsmodelsplugin
    URI "Qt.labs.qmlmodels"
    VERSION "${CMAKE_PROJECT_VERSION}"
    CLASSNAME QtQmlLabsModelsPlugin
    SKIP_TYPE_REGISTRATION
    PLUGIN_OPTIONAL
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::QmlPrivate
        Qt::LabsQmlModelsPrivate
)

#### Keys ignored in scope 1:.:.:labsmodels.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "Qt/labs/qmlmodels"

## Scopes:
#####################################################################
