# Generated from models.pro.

#####################################################################
## modelsplugin Plugin:
#####################################################################

qt_add_qml_module(modelsplugin
    TARGET_PATH "QtQml/Models.2"
    URI "QtQml.Models"
    VERSION "2.15"
    DESIGNER_SUPPORTED
    CLASSNAME QtQmlModelsPlugin
    SKIP_TYPE_REGISTRATION
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
# IMPORT_VERSION = "2.15"
# TARGETPATH = "QtQml/Models.2"
