# Generated from qtquick2.pro.

#####################################################################
## qtquick2plugin Plugin:
#####################################################################

add_qml_module(qtquick2plugin
    TARGET_PATH "QtQuick.2"
    URI "QtQuick"
    VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick2Plugin
    IMPORTS
        QtQml
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::QmlModels
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:qtquick2.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# TARGETPATH = "QtQuick.2"

## Scopes:
#####################################################################

extend_target(qtquick2plugin CONDITION QT_FEATURE_qml_worker_script
    LIBRARIES
        Qt::QmlWorkerScriptPrivate
    PUBLIC_LIBRARIES
        Qt::QmlWorkerScript
)
