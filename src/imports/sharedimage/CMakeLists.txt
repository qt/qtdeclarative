# Generated from sharedimage.pro.

#####################################################################
## sharedimageplugin Plugin:
#####################################################################

qt_internal_add_qml_module(sharedimageplugin
    URI "Qt.labs.sharedimage"
    VERSION "${CMAKE_PROJECT_VERSION}"
    CLASSNAME QtQuickSharedImagePlugin
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        plugin.cpp
        qsharedimageloader.cpp qsharedimageloader_p.h
        sharedimageprovider.cpp sharedimageprovider.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:sharedimage.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "Qt/labs/sharedimage"
