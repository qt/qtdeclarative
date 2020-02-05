# Generated from sharedimage.pro.

#####################################################################
## sharedimageplugin Plugin:
#####################################################################

qt_add_qml_module(sharedimageplugin
    URI "Qt.labs.sharedimage"
    VERSION "1.0"
    CLASSNAME QtQuickSharedImagePlugin
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
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
# IMPORT_VERSION = "1.0"
# TARGETPATH = "Qt/labs/sharedimage"
