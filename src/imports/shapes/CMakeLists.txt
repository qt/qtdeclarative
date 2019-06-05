# Generated from shapes.pro.

#####################################################################
## qmlshapesplugin Plugin:
#####################################################################

add_qml_module(qmlshapesplugin
    TARGET_PATH "QtQuick/Shapes"
    IMPORT_VERSION "1.${CMAKE_PROJECT_VERSION_MINOR}"
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::QuickPrivate
        Qt::QuickShapesPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickShapes
)

#### Keys ignored in scope 1:.:.:shapes.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.$$QT_MINOR_VERSION"
# TARGETPATH = "QtQuick/Shapes"
# _LOADED = "qml_plugin"
