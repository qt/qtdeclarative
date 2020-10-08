# Generated from qmldom.pro.

#####################################################################
## QmlCompiler Module:
#####################################################################

qt_internal_add_module(QmlDom
    STATIC
    INTERNAL_MODULE
    SOURCES
       qqmldom_global.h
       qqmldomconstants_p.h
       qqmldomerrormessage.cpp qqmldomerrormessage_p.h
       qqmldomitem.cpp qqmldomitem_p.h
       qqmldompath.cpp qqmldompath_p.h
       qqmldomstringdumper.cpp qqmldomstringdumper_p.h
    DEFINES
       QMLDOM_LIBRARY
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlDevToolsPrivate
)

#### Keys ignored in scope 1:.:.:qmlcompiler.pro:<TRUE>:
# _OPTION = "host_build"
