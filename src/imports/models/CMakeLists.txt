# Generated from models.pro.

#####################################################################
## modelsplugin Plugin:
#####################################################################

qt_internal_add_qml_module(modelsplugin
    URI "QtQml.Models"
    VERSION "${CMAKE_PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQmlModelsPlugin
    SKIP_TYPE_REGISTRATION
    PLUGIN_OPTIONAL
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:models.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQml/Models"
