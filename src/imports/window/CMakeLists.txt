# Generated from window.pro.

#####################################################################
## windowplugin Plugin:
#####################################################################

add_qml_module(windowplugin
    CPP_PLUGIN
    TARGET_PATH "QtQuick/Window.2"
    URI "QtQuick.Window"
    VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:window.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# TARGETPATH = "QtQuick/Window.2"
# _LOADED = "qml_plugin"
