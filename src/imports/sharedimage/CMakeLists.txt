# Generated from sharedimage.pro.

#####################################################################
## sharedimageplugin Plugin:
#####################################################################

add_qml_module(sharedimageplugin
    CPP_PLUGIN
    URI "Qt.labs.sharedimage"
    VERSION "1.0"
    SOURCES
        plugin.cpp
        qsharedimageloader.cpp qsharedimageloader_p.h
        sharedimageprovider.cpp sharedimageprovider.h
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:sharedimage.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.0"
# TARGETPATH = "Qt/labs/sharedimage"
# _LOADED = "qml_plugin"
