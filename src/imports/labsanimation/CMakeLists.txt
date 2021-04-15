# Generated from labsanimation.pro.

#####################################################################
## labsanimationplugin Plugin:
#####################################################################

qt_internal_add_qml_module(labsanimationplugin
    URI "Qt.labs.animation"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtLabsAnimationPlugin
    SKIP_TYPE_REGISTRATION
    PLUGIN_OPTIONAL
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::LabsAnimationPrivate
)

#### Keys ignored in scope 1:.:.:labsanimation.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "Qt/labs/animation"
