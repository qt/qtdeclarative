# Generated from wavefrontmesh.pro.

#####################################################################
## qmlwavefrontmeshplugin Plugin:
#####################################################################

qt_add_qml_module(qmlwavefrontmeshplugin
    URI "Qt.labs.wavefrontmesh"
    VERSION "1.15"
    CLASSNAME QmlWavefrontMeshPlugin
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
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
# IMPORT_VERSION = "1.15"
# TARGETPATH = "Qt/labs/wavefrontmesh"
