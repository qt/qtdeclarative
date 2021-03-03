# Generated from layouts.pro.

#####################################################################
## qquicklayoutsplugin Plugin:
#####################################################################


qt_internal_add_qml_module(qquicklayoutsplugin
    URI "QtQuick.Layouts"
    VERSION "${CMAKE_PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASS_NAME QtQuickLayoutsPlugin
    SKIP_TYPE_REGISTRATION
    DEPENDENCIES
        QtQuick
    PLUGIN_OPTIONAL
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::QuickLayoutsPrivate
)


#### Keys ignored in scope 1:.:.:layouts.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick/Layouts"
