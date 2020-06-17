# Generated from qtqml.pro.

set(module_dynamic_qml_imports
    QtQml.Models/auto
)

if (QT_FEATURE_qml_worker_script)
    list(APPEND module_dynamic_qml_imports
        QtQml.WorkerScript/auto
    )
endif()

#####################################################################
## qmlplugin Plugin:
#####################################################################

qt_add_qml_module(qmlplugin
    URI "QtQml"
    VERSION "${CMAKE_PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQmlPlugin
    SKIP_TYPE_REGISTRATION
    IMPORTS ${module_dynamic_qml_imports}
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
        Qt::CorePrivate # special case
)

#### Keys ignored in scope 1:.:.:qtqml.pro:<TRUE>:
# CXX_MODULE = "qml"
# DYNAMIC_QMLDIR = "module QtQml" "plugin qmlplugin" "classname QtQmlPlugin" "typeinfo plugins.qmltypes" "designersupported" "import QtQml.Models"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQml"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qtqml.pro:QT_FEATURE_qml_worker_script:
# DYNAMIC_QMLDIR = "import QtQml.WorkerScript"
