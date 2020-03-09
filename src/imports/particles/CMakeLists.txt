# Generated from particles.pro.

#####################################################################
## particlesplugin Plugin:
#####################################################################

qt_add_qml_module(particlesplugin
    URI "QtQuick.Particles"
    VERSION "${CMAKE_PROJECT_VERSION}"
    CLASSNAME QtQuick2ParticlesPlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::QmlPrivate
        Qt::QuickParticlesPrivate
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:particles.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick/Particles"
