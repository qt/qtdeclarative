# Generated from templates.pro.

#####################################################################
## qtquicktemplates2plugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquicktemplates2plugin
    URI "QtQuick.Templates"
    VERSION "${PROJECT_VERSION}"
    CLASSNAME QtQuickTemplates2Plugin
    DEPENDENCIES
        QtQuick/auto
    SKIP_TYPE_REGISTRATION
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
# OTHER_FILES = "qmldir"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick/Templates"
