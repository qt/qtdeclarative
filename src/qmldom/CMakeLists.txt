# Generated from qmldom.pro.

#####################################################################
## QmlDom Module:
#####################################################################

qt_internal_add_module(QmlDom
    STATIC
    INTERNAL_MODULE
    # NO_SYNC_QT # special case remove, it's actually needed for tests
    SOURCES
        qqmldom_fwd_p.h
        qqmldom_global.h
        qqmldomattachedinfo.cpp qqmldomattachedinfo_p.h
        qqmldomconstants_p.h
        qqmldomerrormessage.cpp qqmldomerrormessage_p.h
        qqmldomexternalitems.cpp qqmldomexternalitems_p.h
        qqmldomfunctionref_p.h
        qqmldomitem.cpp qqmldomitem_p.h
        qqmldompath.cpp qqmldompath_p.h
        qqmldomstringdumper.cpp qqmldomstringdumper_p.h
        qqmldomtop.cpp qqmldomtop_p.h
    DEFINES
        QMLDOM_LIBRARY
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlDevToolsPrivate
)

#### Keys ignored in scope 1:.:.:qmldom.pro:<TRUE>:
# _OPTION = "host_build"
