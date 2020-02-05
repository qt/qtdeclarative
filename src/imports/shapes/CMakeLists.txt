# Generated from shapes.pro.

#####################################################################
## qmlshapesplugin Plugin:
#####################################################################

qt_add_qml_module(qmlshapesplugin
    URI "QtQuick.Shapes"
    VERSION "1.15"
    CLASSNAME QmlShapesPlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::GuiPrivate
        Qt::Qml
        Qt::QuickPrivate
        Qt::QuickShapesPrivate
)

#### Keys ignored in scope 1:.:.:shapes.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.15"
# TARGETPATH = "QtQuick/Shapes"
