# Generated from qmldiskcache.pro.

#####################################################################
## tst_qmldiskcache Test:
#####################################################################

add_qt_test(tst_qmldiskcache
    GUI
    SOURCES
        tst_qmldiskcache.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
)

# special case begin
# Disable quick compiler for this test
set_source_files_properties(
    importmodule.qml
    module.mjs
    test.qml
    PROPERTIES
    QT_SKIP_QUICKCOMPILER TRUE
)
# special case end

# Resources:
add_qt_resource(tst_qmldiskcache "qmake_immediate"
    PREFIX
        "/"
    FILES
        importmodule.qml
        module.mjs
        test.qml
)


#### Keys ignored in scope 1:.:.:qmldiskcache.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qmldiskcache.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
