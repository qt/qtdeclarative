# Generated from wavefrontmesh.pro.

#####################################################################
## qmlwavefrontmeshplugin Plugin:
#####################################################################

add_qml_module(qmlwavefrontmeshplugin
    URI "Qt.labs.wavefrontmesh"
    VERSION "1.${CMAKE_PROJECT_VERSION_MINOR}"
    CLASSNAME QmlWavefrontMeshPlugin
    SOURCES
        plugin.cpp
        qwavefrontmesh.cpp qwavefrontmesh.h
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:wavefrontmesh.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.$$QT_MINOR_VERSION"
# TARGETPATH = "Qt/labs/wavefrontmesh"
# _LOADED = "qml_plugin"
