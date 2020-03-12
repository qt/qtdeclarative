# Generated from qmldiskcache.pro.

#####################################################################
## tst_qmldiskcache Test:
#####################################################################

qt_add_test(tst_qmldiskcache
    SOURCES
        tst_qmldiskcache.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::QmlPrivate
)

# Resources:
set(qmake_immediate_resource_files
    "importmodule.qml"
    "module.mjs"
    "test.qml"
)

qt_add_resource(tst_qmldiskcache "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


## Scopes:
#####################################################################
