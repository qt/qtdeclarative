# Generated from sharedimage.pro.

#####################################################################
## sharedimageplugin Plugin:
#####################################################################

qt_internal_add_qml_module(sharedimageplugin
    URI "Qt.labs.sharedimage"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtQuickSharedImagePlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::LabsSharedImagePrivate
)

#### Keys ignored in scope 1:.:.:sharedimage.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "Qt/labs/sharedimage"
