# Generated from window.pro.

#####################################################################
## windowplugin Plugin:
#####################################################################

qt_add_qml_module(windowplugin
    URI "QtQuick.Window"
    VERSION "2.15"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick2WindowPlugin
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
    SOURCES
        plugin.cpp plugin.h
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
# IMPORT_VERSION = "2.15"
# TARGETPATH = "QtQuick/Window"
