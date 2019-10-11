# Generated from layouts.pro.

#####################################################################
## qquicklayoutsplugin Plugin:
#####################################################################

add_qml_module(qquicklayoutsplugin
    URI "QtQuick.Layouts"
    VERSION "1.${CMAKE_PROJECT_VERSION_MINOR}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuickLayoutsPlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
        qquickgridlayoutengine.cpp qquickgridlayoutengine_p.h
        qquicklayout.cpp qquicklayout_p.h
        qquicklayoutstyleinfo.cpp qquicklayoutstyleinfo_p.h
        qquicklinearlayout.cpp qquicklinearlayout_p.h
        qquickstacklayout.cpp qquickstacklayout_p.h
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:layouts.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.$$QT_MINOR_VERSION"
# TARGETPATH = "QtQuick/Layouts"
# _LOADED = "qml_plugin"
