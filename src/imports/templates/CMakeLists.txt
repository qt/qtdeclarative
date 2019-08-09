# Generated from templates.pro.

#####################################################################
## qtquicktemplates2plugin Plugin:
#####################################################################

add_qml_module(qtquicktemplates2plugin
    CPP_PLUGIN
    TARGET_PATH "QtQuick/Templates.2"
    URI "QtQuick.Templates"
    VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    SOURCES
        qtquicktemplates2plugin.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:templates.pro:<TRUE>:
# CONFIG = "no_cxx_module"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# OTHER_FILES = "qmldir"
# TARGETPATH = "QtQuick/Templates.2"
# _LOADED = "qml_plugin"
