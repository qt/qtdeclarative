# Generated from qtquick2.pro.

#####################################################################
## qtquick2plugin Plugin:
#####################################################################

qt_add_qml_module(qtquick2plugin
    URI "QtQuick"
    VERSION "${CMAKE_PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick2Plugin
    SKIP_TYPE_REGISTRATION
    IMPORTS
        QtQml/auto
    PLUGIN_OPTIONAL
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:qtquick2.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick"

## Scopes:
#####################################################################

qt_extend_target(qtquick2plugin CONDITION QT_FEATURE_qml_worker_script
    PUBLIC_LIBRARIES
        Qt::QmlWorkerScriptPrivate
)
