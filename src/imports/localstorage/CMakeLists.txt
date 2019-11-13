# Generated from localstorage.pro.

#####################################################################
## qmllocalstorageplugin Plugin:
#####################################################################

add_qml_module(qmllocalstorageplugin
    URI "QtQuick.LocalStorage"
    VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    CLASSNAME QQmlLocalStoragePlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
        qquicklocalstorage.cpp qquicklocalstorage_p.h
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::Sql
)

#### Keys ignored in scope 1:.:.:localstorage.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# OTHER_FILES = "localstorage.json"
# TARGETPATH = "QtQuick/LocalStorage"
