# Generated from labsanimation.pro.

#####################################################################
## labsanimationplugin Plugin:
#####################################################################

add_qml_module(labsanimationplugin
    INSTALL_QML_FILES
    TARGET_PATH "Qt/labs/animation"
    IMPORT_VERSION "1.0"
    SOURCES
        plugin.cpp
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
# _LOADED = "qml_plugin"
