# Generated from folderlistmodel.pro.

#####################################################################
## qmlfolderlistmodelplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qmlfolderlistmodelplugin
    URI "Qt.labs.folderlistmodel"
    VERSION "${CMAKE_PROJECT_VERSION}"
    CLASS_NAME QmlFolderListModelPlugin
    SKIP_TYPE_REGISTRATION
    PLUGIN_OPTIONAL
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::LabsFolderListModelPrivate
)

#### Keys ignored in scope 1:.:.:folderlistmodel.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "Qt/labs/folderlistmodel"
