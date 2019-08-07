# Generated from qjsengine.pro.

#####################################################################
## tst_qjsengine Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "script/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qjsengine
    GUI
    SOURCES
        tst_qjsengine.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Widgets
    TESTDATA ${test_data}
)

# Resources:
add_qt_resource(tst_qjsengine "qjsengine"
    PREFIX
        "/"
    FILES
        translations/idtranslatable-unicode.qm
        translations/idtranslatable_la.qm
        translations/translatable-unicode.qm
        translations/translatable_la.qm
)
add_qt_resource(tst_qjsengine "qmake_immediate"
    PREFIX
        "/"
    FILES
        exporterror1.mjs
        importerror1.mjs
        modulewithlexicals.mjs
        testmodule.mjs
)


#### Keys ignored in scope 1:.:.:qjsengine.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qjsengine.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
