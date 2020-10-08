# Generated from labsanimation.pro.

#####################################################################
## labsanimationplugin Plugin:
#####################################################################

qt_internal_add_qml_module(labsanimationplugin
    URI "Qt.labs.animation"
    VERSION "${CMAKE_PROJECT_VERSION}"
    CLASSNAME QtLabsAnimationPlugin
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        plugin.cpp
        qquickboundaryrule.cpp qquickboundaryrule_p.h
    PUBLIC_LIBRARIES
        Qt::QmlPrivate
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:labsanimation.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "Qt/labs/animation"
