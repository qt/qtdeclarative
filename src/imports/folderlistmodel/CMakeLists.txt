# Generated from folderlistmodel.pro.

#####################################################################
## qmlfolderlistmodelplugin Plugin:
#####################################################################

qt_add_qml_module(qmlfolderlistmodelplugin
    URI "Qt.labs.folderlistmodel"
    VERSION "2.15"
    CLASSNAME QmlFolderListModelPlugin
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
    SOURCES
        fileinfothread.cpp fileinfothread_p.h
        fileproperty_p.h
        plugin.cpp
        qquickfolderlistmodel.cpp qquickfolderlistmodel.h
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:folderlistmodel.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "2.15"
# TARGETPATH = "Qt/labs/folderlistmodel"
