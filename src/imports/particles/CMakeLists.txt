# Generated from particles.pro.

#####################################################################
## particlesplugin Plugin:
#####################################################################

qt_add_qml_module(particlesplugin
    TARGET_PATH "QtQuick/Particles.2"
    URI "QtQuick.Particles"
    VERSION "2.15"
    CLASSNAME QtQuick2ParticlesPlugin
    SKIP_TYPE_REGISTRATION
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
# IMPORT_VERSION = "2.15"
# TARGETPATH = "QtQuick/Particles.2"
