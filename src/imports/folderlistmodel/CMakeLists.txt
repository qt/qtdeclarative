# Generated from folderlistmodel.pro.

#####################################################################
## qmlfolderlistmodelplugin Plugin:
#####################################################################

add_qml_module(qmlfolderlistmodelplugin
    TARGET_PATH "Qt/labs/folderlistmodel"
    IMPORT_VERSION "2.${CMAKE_PROJECT_VERSION_MINOR}"
    SOURCES
        fileinfothread.cpp fileinfothread_p.h
        fileproperty_p.h
        plugin.cpp
        qquickfolderlistmodel.cpp qquickfolderlistmodel.h
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:folderlistmodel.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "2.$$QT_MINOR_VERSION"
# TARGETPATH = "Qt/labs/folderlistmodel"
# _LOADED = "qml_plugin"
