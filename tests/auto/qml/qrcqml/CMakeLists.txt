# Generated from qrcqml.pro.

#####################################################################
## tst_qrcqml Test:
#####################################################################

add_qt_test(tst_qrcqml
    GUI
    SOURCES
        tst_qrcqml.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
)

# Resources:
set_source_files_properties("data/SameDir2.qml"
    PROPERTIES QT_RESOURCE_ALIAS "SameDir.qml"
)
set_source_files_properties("data/importtest.qml"
    PROPERTIES QT_RESOURCE_ALIAS "importtest.qml"
)
set_source_files_properties("data/main2.qml"
    PROPERTIES QT_RESOURCE_ALIAS "main.qml"
)
add_qt_resource(tst_qrcqml "qrcqml"
    PREFIX
        "/"
    FILES
        data/SameDir.qml
        data/SameDir2.qml
        data/SubDir.qml
        data/importtest.qml
        data/main.qml
        data/main2.qml
)
set_source_files_properties("data/SameDir3.qml"
    PROPERTIES QT_RESOURCE_ALIAS "SameDir.qml"
)
set_source_files_properties("data/main3.qml"
    PROPERTIES QT_RESOURCE_ALIAS "main.qml"
)
add_qt_resource(tst_qrcqml "qrcqml1"
    PREFIX
        "/search"
    FILES
        data/SameDir3.qml
        data/main3.qml
)
set_source_files_properties("data/SubDir.qml"
    PROPERTIES QT_RESOURCE_ALIAS "SubDir.qml"
)
add_qt_resource(tst_qrcqml "qrcqml2"
    PREFIX
        "/search/data"
    FILES
        data/SubDir.qml
)
set_source_files_properties("data/imports/QrcImport/Imported.qml"
    PROPERTIES QT_RESOURCE_ALIAS "QrcImport/Imported.qml"
)
set_source_files_properties("data/imports/QrcImport/qmldir"
    PROPERTIES QT_RESOURCE_ALIAS "QrcImport/qmldir"
)
add_qt_resource(tst_qrcqml "qrcqml3"
    PREFIX
        "/imports"
    FILES
        data/imports/QrcImport/Imported.qml
        data/imports/QrcImport/qmldir
)


#### Keys ignored in scope 1:.:.:qrcqml.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qrcqml.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
