# Generated from settings.pro.

#####################################################################
## qmlsettingsplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qmlsettingsplugin
    URI "Qt.labs.settings"
    VERSION "${PROJECT_VERSION}"
    CLASSNAME QmlSettingsPlugin
    SKIP_TYPE_REGISTRATION
    PLUGIN_OPTIONAL
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::LabsSettingsPrivate
)

#### Keys ignored in scope 1:.:.:settings.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "Qt/labs/settings"
