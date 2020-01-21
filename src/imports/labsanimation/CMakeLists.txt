# Generated from labsanimation.pro.

#####################################################################
## labsanimationplugin Plugin:
#####################################################################

qt_add_qml_module(labsanimationplugin
    URI "Qt.labs.animation"
    VERSION "1.0"
    CLASSNAME QtLabsAnimationPlugin
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
    SOURCES
        plugin.cpp
        qquickboundaryrule.cpp qquickboundaryrule_p.h
    LIBRARIES
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:labsanimation.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.0"
# TARGETPATH = "Qt/labs/animation"
