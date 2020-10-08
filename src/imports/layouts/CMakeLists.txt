# Generated from layouts.pro.

#####################################################################
## qquicklayoutsplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qquicklayoutsplugin
    URI "QtQuick.Layouts"
    VERSION "${CMAKE_PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuickLayoutsPlugin
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        plugin.cpp
        qquickgridlayoutengine.cpp qquickgridlayoutengine_p.h
        qquicklayout.cpp qquicklayout_p.h
        qquicklayoutstyleinfo.cpp qquicklayoutstyleinfo_p.h
        qquicklinearlayout.cpp qquicklinearlayout_p.h
        qquickstacklayout.cpp qquickstacklayout_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:layouts.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick/Layouts"
