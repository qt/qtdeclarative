# Generated from qtquick2.pro.

#####################################################################
## qtquick2plugin Plugin:
#####################################################################

qt_add_qml_module(qtquick2plugin
    URI "QtQuick"
    VERSION "2.15"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick2Plugin
    SKIP_TYPE_REGISTRATION
    IMPORTS
        QtQml
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
# IMPORT_VERSION = "2.15"
# TARGETPATH = "QtQuick"

## Scopes:
#####################################################################

qt_extend_target(qtquick2plugin CONDITION QT_FEATURE_qml_worker_script
    LIBRARIES
        Qt::QmlWorkerScriptPrivate
    PUBLIC_LIBRARIES
        Qt::QmlWorkerScript
)
