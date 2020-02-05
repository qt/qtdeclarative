# Generated from particles.pro.

#####################################################################
## particlesplugin Plugin:
#####################################################################

qt_add_qml_module(particlesplugin
    URI "QtQuick.Particles"
    VERSION "2.15"
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
# IMPORT_VERSION = "2.15"
# TARGETPATH = "QtQuick/Particles"
