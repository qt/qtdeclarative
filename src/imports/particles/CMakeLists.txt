# Generated from particles.pro.

#####################################################################
## particlesplugin Plugin:
#####################################################################

add_qml_module(particlesplugin
    CPP_PLUGIN
    URI "QtQuick.Particles.2"
    TARGET_PATH "QtQuick/Particles.2"
    VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    RESOURCE_PREFIX "/qt-project.org/imports"
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::QmlPrivate
        Qt::QuickParticlesPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickParticles
)

#### Keys ignored in scope 1:.:.:particles.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# TARGETPATH = "QtQuick/Particles.2"
# _LOADED = "qml_plugin"
