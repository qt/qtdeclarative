# Generated from particles.pro.

#####################################################################
## particlesplugin Plugin:
#####################################################################

add_qml_module(particlesplugin
    INSTALL_QML_FILES
    TARGET_PATH "QtQuick/Particles.2"
    IMPORT_VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::QmlPrivate
        Qt::QuickParticlesPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Quick
        Qt::QuickParticles
)

#### Keys ignored in scope 1:.:.:particles.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# TARGETPATH = "QtQuick/Particles.2"
# _LOADED = "qml_plugin"
